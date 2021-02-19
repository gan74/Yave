/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "EditorContext.h"
#include <yave/graphics/device/Device.h>

#include <yave/assets/SQLiteAssetStore.h>
#include <yave/assets/FolderAssetStore.h>
#include <yave/assets/AssetLoader.h>

#include <y/io2/File.h>

#include <thread>

namespace editor {

DevicePtr ContextLinked::device() const {
    return _ctx ? _ctx->device() : nullptr;
}


static constexpr std::string_view world_file = "../world.yw3";
static constexpr std::string_view store_file = "../store.sqlite3";
static constexpr std::string_view store_dir = "../store";

EditorContext::EditorContext(DevicePtr dptr) :
        DeviceLinked(dptr),
        _resources(dptr),
        _asset_store(std::make_shared<SQLiteAssetStore>(store_file)),
        //_asset_store(std::make_shared<FolderAssetStore>(store_dir)),
        _loader(std::make_unique<AssetLoader>(dptr, _asset_store/*, AssetLoadingFlags::SkipFailedDependenciesBit*/)),
        _scene_view(&_default_scene_view),
        _ui_manager(this),
        _notifs(this),
        _thumb_cache(this),
        _world(this) {


    load_world();
}

EditorContext::~EditorContext() {
}

void EditorContext::start_perf_capture() {
    _perf_capture_frames = settings().perf().capture_forever ? u32(-1) : settings().perf().capture_frames;
}

void EditorContext::end_perf_capture() {
    if(_perf_capture_frames) {
        _perf_capture_frames = 1;
    }
}

void EditorContext::reload_device_resources() {
    _reload_resources = true;
}

void EditorContext::set_device_resource_reloaded() {
    y_debug_assert(_reload_resources);
    _reload_resources = false;
}

bool EditorContext::device_resources_reload_requested() const {
    return _reload_resources;
}

void EditorContext::flush_reload() {
    defer([this]() {
        y_profile_zone("flush reload");
        _thumb_cache.clear();
        _selection.flush_reload();
        _ui_manager.refresh_all();
        _world.flush_reload();
    });
}

void EditorContext::defer(std::function<void()> func) {
    const auto lock = y_profile_unique_lock(_deferred_lock);
    if(_is_flushing_deferred) {
        y_fatal("Defer called from already deferred function");
    }
    _deferred.emplace_back(std::move(func));
}

void EditorContext::flush_deferred() {
    y_profile();
    const auto lock = y_profile_unique_lock(_deferred_lock);
    if(!_deferred.is_empty()) {
        device()->wait_all_queues();
        _is_flushing_deferred = true;
        for(auto& f : _deferred) {
            f();
        }
        _deferred.clear();
        _is_flushing_deferred = false;
    }
    //_world.flush();

    if(_perf_capture_frames) {
        if(perf::is_capturing()) {
            if(--_perf_capture_frames == 0) {
                perf::end_capture();
            }
        } else {
            perf::start_capture(settings().perf().capture_name);
        }
    }
}

void EditorContext::set_scene_view(SceneView* scene) {
    if(!scene) {
        _scene_view = &_default_scene_view;
    } else {
        _scene_view = scene;
    }
}

void EditorContext::remove_scene_view(SceneView* scene) {
    if(_scene_view == scene) {
        set_scene_view(nullptr);
    }
}

SceneView& EditorContext::scene_view() {
    return *_scene_view;
}

SceneView& EditorContext::default_scene_view() {
    return _default_scene_view;
}

EditorWorld& EditorContext::world() {
    return _world;
}

const FileSystemModel* EditorContext::filesystem() const {
    return _filesystem.get() ? _filesystem.get() : FileSystemModel::local_filesystem();
}

const EditorResources& EditorContext::resources() const {
    return _resources;
}

EditorResources& EditorContext::resources() {
    return _resources;
}

Settings& EditorContext::settings() {
    return _settings;
}

Selection& EditorContext::selection() {
    return _selection;
}

AssetLoader& EditorContext::loader() {
    return *_loader;
}

UiManager& EditorContext::ui_manager() {
    return _ui_manager;
}

ThumbmailCache& EditorContext::thumbmail_cache() {
    return _thumb_cache;
}

AssetStore& EditorContext::asset_store() {
    return *_asset_store;
}

Notifications& EditorContext::notifications() {
    return _notifs;
}

void EditorContext::save_world() const {
    y_profile();
    auto file = io2::File::create(world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }
    serde3::WritableArchive arc(file.unwrap());
    if(auto r = arc.serialize(_world); !r) {
        log_msg(fmt("Unable to save world: %", serde3::error_msg(r.error())), Log::Error);
    }
}

void EditorContext::load_world() {
    y_profile();

    auto file = io2::File::open(world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }

    EditorWorld world(this);
    {
        serde3::ReadableArchive arc(file.unwrap());
        const auto status = arc.deserialize(world);
        if(status.is_error()) {
            log_msg(fmt("Unable to load world: % (for %)", serde3::error_msg(status.error()), status.error().member), Log::Error);
            world = EditorWorld(this);
        } else if(status.unwrap() == serde3::Success::Partial) {
            log_msg("World was only partialy loaded", Log::Warning);
        }
    }

    _world = std::move(world);
    y_debug_assert(_world.required_components().size() == 1);
}

void EditorContext::new_world() {
    _world = EditorWorld(this);
}

}


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

#include "EditorApplication.h"
#include "EditorWorld.h"

#include "EditorResources.h"
#include "UiManager.h"
#include "ImGuiPlatform.h"

#include <yave/assets/SQLiteAssetStore.h>
#include <yave/assets/AssetLoader.h>

#include <y/io2/File.h>
#include <y/serde3/archives.h>
#include <y/utils/log.h>

namespace editor {


static constexpr std::string_view world_file = "../world.yw3";
static constexpr std::string_view store_file = "../store.sqlite3";

EditorApplication* EditorApplication::_instance = nullptr;


EditorApplication* EditorApplication::instance() {
    y_debug_assert(_instance);
    return _instance;
}

EditorApplication::EditorApplication(DevicePtr dptr) : DeviceLinked(dptr) {
    y_always_assert(_instance == nullptr, "Editor instance already exists.");
    _instance = this;

    _resources = std::make_unique<EditorResources>(dptr);

    _platform = std::make_unique<ImGuiPlatform>(dptr);
    _ui = std::make_unique<UiManager>();

    _asset_store = std::make_shared<SQLiteAssetStore>(store_file);
    _loader = std::make_unique<AssetLoader>(dptr, _asset_store);
    _world = std::make_unique<EditorWorld>(*_loader);

    _default_scene_view = SceneView(_world.get());
    _scene_view = &_default_scene_view;

    load_world();
}

EditorApplication::~EditorApplication() {
    y_always_assert(_instance == this, "Editor instance isn't set propertly.");
}

void EditorApplication::exec() {
    _platform->exec([this](CmdBufferRecorder& rec) {
        y_debug_assert(!_recorder);
        _recorder = &rec;
        _ui->on_gui();
        _recorder = nullptr;
    });
}

void EditorApplication::set_scene_view(SceneView* scene) {
    if(!scene) {
        _scene_view = &_default_scene_view;
    } else {
        _scene_view = scene;
    }
}

void EditorApplication::unset_scene_view(SceneView* scene) {
    if(_scene_view == scene) {
        set_scene_view(nullptr);
    }
}


void EditorApplication::load_world() {
    y_profile();

    auto file = io2::File::open(world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }

    EditorWorld world(*_loader);

    serde3::ReadableArchive arc(file.unwrap());
    const auto status = arc.deserialize(world);
    if(status.is_error()) {
        log_msg(fmt("Unable to load world: % (for %)", serde3::error_msg(status.error()), status.error().member), Log::Error);
        return;
    }

    if(status.unwrap() == serde3::Success::Partial) {
        log_msg("World was only partialy loaded", Log::Warning);
    }

    *_world = std::move(world);
}

}

/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "Settings.h"
#include "EditorResources.h"
#include "UiManager.h"
#include "ImGuiPlatform.h"
#include "ThumbmailRenderer.h"
#include "UndoStack.h"

#include <yave/assets/FolderAssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/DirectDraw.h>

#include <y/io2/File.h>
#include <y/serde3/archives.h>
#include <y/utils/log.h>
#include <y/test/test.h>

#include <external/imgui/IconsFontAwesome5.h>

namespace editor {


#ifdef Y_DEBUG
editor_action_desc("Debug assert", "Calls assert(false) and crashes the program", [] { y_debug_assert(false); })
#endif

editor_action("Quit", [] { imgui_platform()->main_window()->close(); })
editor_action("Show ImGui demo", [] { imgui_platform()->show_demo(); })
editor_action("Restore default layout", [] { ui().restore_default_layout(); })

editor_action_desc("Lag", "Pause execution for 1s to simulate load", [] { core::Duration::sleep(core::Duration::seconds(1)); })

editor_action_shortcut(ICON_FA_SAVE " Save", Key::Ctrl + Key::S, [] { application()->save_world(); }, "File")
editor_action(ICON_FA_FOLDER " Load", [] { application()->load_world(); }, "File")
editor_action_shortcut("New", Key::Ctrl + Key::N, [] { application()->new_world(); }, "File")


EditorApplication* EditorApplication::_instance = nullptr;

EditorApplication* EditorApplication::instance() {
    y_debug_assert(_instance);
    return _instance;
}

EditorApplication::EditorApplication(ImGuiPlatform* platform) : _platform(platform) {
    y_always_assert(_instance == nullptr, "Editor instance already exists.");
    _instance = this;


    const bool running_tests = platform->is_running_tests();
    const auto& store_dir = running_tests
        ? app_settings().editor.test_asset_store
        : app_settings().editor.asset_store;

    _world_file = running_tests
        ? app_settings().editor.test_world_file
        : app_settings().editor.world_file;

    _resources = std::make_unique<EditorResources>();
    _ui = std::make_unique<UiManager>();

    _asset_store = std::make_shared<FolderAssetStore>(store_dir);
    _loader = std::make_unique<AssetLoader>(_asset_store, AssetLoadingFlags::SkipFailedDependenciesBit, 4);
    _thumbmail_renderer = std::make_unique<ThumbmailRenderer>(*_loader);

    _world = std::make_unique<EditorWorld>(*_loader);
    _debug_drawer = std::make_unique<DirectDraw>();

    _undo_stack = std::make_unique<UndoStack>();

    _default_scene_view = SceneView(_world.get());
    _scene_view = &_default_scene_view;

    load_world_deferred();
}

EditorApplication::~EditorApplication() {
    _ui->close_all();

    // Close threads before unsetting the instance
    _thumbmail_renderer = nullptr;

    y_always_assert(_instance == this, "Editor instance has already been deleted.");
    _instance = nullptr;
}

void EditorApplication::exec() {
    _platform->exec([this](CmdBufferRecorder& rec) {
        y_debug_assert(!_recorder);
        _recorder = &rec;
        _world->tick();
        _world->update(float(_update_timer.reset().to_secs()));
        _ui->on_gui();
        process_deferred_actions();
        _recorder = nullptr;
    });
}

void EditorApplication::flush_reload() {
    log_msg("flush_reload not implemented", Log::Warning);
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


void EditorApplication::save_world() {
    _deferred_actions |= Save;
}

void EditorApplication::load_world() {
    _deferred_actions |= Load;
}

void EditorApplication::new_world() {
    _deferred_actions |= New;
}

void EditorApplication::process_deferred_actions() {

    if(_deferred_actions & Save) {
        save_world_deferred();
    }
    if(_deferred_actions & Load) {
        load_world_deferred();
    }
    if(_deferred_actions & New) {
        _world = std::make_unique<EditorWorld>(*_loader);
        log_msg("New world");
    }

    _deferred_actions = None;
}

void EditorApplication::save_world_deferred() const {
    y_profile();

    auto file = io2::File::create(_world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }

    serde3::WritableArchive arc(file.unwrap());
    if(auto r = _world->save_state(arc); !r) {
        log_msg(fmt("Unable to save world: {}", serde3::error_msg(r.error())), Log::Error);
        return;
    }

    log_msg("World saved");
}

void EditorApplication::load_world_deferred() {
    y_profile();

    auto file = io2::File::open(_world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }

    serde3::ReadableArchive arc(file.unwrap(), serde3::DeserializationFlags::DontPropagatePolyFailure);
    if(auto r = _world->load_state(arc); !r) {
        log_msg(fmt("Unable to load world: {} (for {})", serde3::error_msg(r.error()), r.error().member), Log::Error);
        return;
    } else if(r.unwrap() == serde3::Success::Partial) {
        log_msg("World was only partialy loaded", Log::Warning);
    }

    log_msg("World loaded");
}

}


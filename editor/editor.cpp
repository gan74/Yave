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

#include "editor.h"

#include "EditorResources.h"
#include "UiManager.h"
#include "ImGuiPlatform.h"
#include "ThumbmailRenderer.h"
#include "UndoStack.h"
#include "EditorWorld.h"

#include <yave/assets/FolderAssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/DirectDraw.h>
#include <yave/scene/SceneView.h>

#include <y/io2/File.h>
#include <y/serde3/archives.h>
#include <y/utils/log.h>
#include <y/test/test.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


namespace editor {
namespace application {
std::unique_ptr<EditorResources> resources;
std::shared_ptr<AssetStore> asset_store;
std::unique_ptr<AssetLoader> loader;
std::unique_ptr<ThumbmailRenderer> thumbmail_renderer;
std::unique_ptr<DirectDraw> debug_drawer;
std::unique_ptr<UiManager> ui;
std::unique_ptr<EditorWorld> world;

core::String world_file;

ImGuiPlatform* imgui_platform = nullptr;

SceneView default_scene_view;
SceneView* scene_view = nullptr;

Settings settings;
core::Chrono update_timer;

enum DeferredActions : u32 {
    None    = 0x00,
    Save    = 0x01,
    Load    = 0x02,
    New     = 0x04,
};

u32 deferred_actions = None;
}


void post_tick();

void init_editor(ImGuiPlatform* platform, const Settings& settings) {
    application::settings = settings;
    application::imgui_platform = platform;

    const bool running_tests = platform->is_running_tests();
    const auto& store_dir = running_tests
        ? app_settings().editor.test_asset_store
        : app_settings().editor.asset_store;

    application::world_file = running_tests
        ? app_settings().editor.test_world_file
        : app_settings().editor.world_file;

    application::resources = std::make_unique<EditorResources>();
    application::ui = std::make_unique<UiManager>();
    application::asset_store = std::make_shared<FolderAssetStore>(store_dir);
    application::loader = std::make_unique<AssetLoader>(application::asset_store, AssetLoadingFlags::SkipFailedDependenciesBit, 4);
    application::thumbmail_renderer = std::make_unique<ThumbmailRenderer>(*application::loader);
    application::world = std::make_unique<EditorWorld>(*application::loader);
    application::debug_drawer = std::make_unique<DirectDraw>();

    application::default_scene_view = SceneView(application::world.get());
    application::scene_view = &application::default_scene_view;

    application::deferred_actions = application::Load;

    post_tick();
}


void destroy_editor() {
    application::thumbmail_renderer = nullptr;
    application::world = nullptr;
    application::scene_view  = nullptr;
    application::default_scene_view = {};
    application::loader = nullptr;
    application::asset_store = nullptr;
    application::debug_drawer = nullptr;
    application::resources = nullptr;
    application::ui = nullptr;
}

void run_editor() {
    application::imgui_platform->exec([](CmdBufferRecorder&) {
        application::world->tick();
        application::world->update(float(application::update_timer.reset().to_secs()));
        application::ui->on_gui();
        post_tick();
    });
}


static void save_world_deferred() {
    y_profile();

    auto file = io2::File::create(application::world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }

    serde3::WritableArchive arc(file.unwrap());
    if(auto r = application::world->save_state(arc); !r) {
        log_msg(fmt("Unable to save world: {}", serde3::error_msg(r.error())), Log::Error);
        return;
    }

    log_msg("World saved");
}

static void load_world_deferred() {
    y_profile();

    auto file = io2::File::open(application::world_file);
    if(!file) {
        log_msg("Unable to open file", Log::Error);
        return;
    }

    serde3::ReadableArchive arc(file.unwrap(), serde3::DeserializationFlags::DontPropagatePolyFailure);
    if(auto r = application::world->load_state(arc); !r) {
        log_msg(fmt("Unable to load world: {} (for {})", serde3::error_msg(r.error()), r.error().member), Log::Error);
        return;
    } else if(r.unwrap() == serde3::Success::Partial) {
        log_msg("World was only partialy loaded", Log::Warning);
    }

    log_msg("World loaded");
}

void post_tick() {
    y_profile();
    if(application::deferred_actions & application::Save) {
    }

    if(application::deferred_actions & application::Load) {
        load_world_deferred();
    }

    if(application::deferred_actions & application::New) {
        application::world = std::make_unique<EditorWorld>(*application::loader);
        application::default_scene_view = SceneView(application::world.get());
        log_msg("New world");
    }

    application::deferred_actions = application::None;
}




Settings& app_settings() {
    return application::settings;
}

UiManager& ui() {
    return *application::ui;
}

AssetStore& asset_store() {
    return *application::asset_store;
}

AssetLoader& asset_loader() {
    return *application::loader;
}

ThumbmailRenderer& thumbmail_renderer() {
    return *application::thumbmail_renderer;
}

const EditorResources& resources() {
    return *application::resources;
}

void save_world() {
    application::deferred_actions |= application::Save;
}

void load_world() {
    application::deferred_actions |= application::Load;
}

void new_world() {
    application::deferred_actions |= application::New;
}

EditorWorld& current_world() {
    return *application::world;
}

void set_scene_view(SceneView* scene) {
    if(!scene) {
        application::scene_view = &application::default_scene_view;
    } else {
        application::scene_view = scene;
    }
}

void unset_scene_view(SceneView* scene) {
    if(application::scene_view == scene) {
        set_scene_view(nullptr);
    }
}

const SceneView& scene_view() {
    return *application::scene_view;
}







DirectDraw& debug_drawer() {
    return *application::debug_drawer;
}








Widget* add_widget(std::unique_ptr<Widget> widget, bool auto_parent) {
    return ui().add_widget(std::move(widget), auto_parent);
}


namespace detail {
EditorAction* first_action = nullptr;

void register_action(EditorAction* action) {
    log_msg(fmt("Registering action \"{}\"", action->name), Log::Debug);
    action->next = first_action;
    first_action = action;
}
}

const EditorAction* all_actions() {
    return detail::first_action;
}

}


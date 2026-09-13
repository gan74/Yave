/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#include "ThumbnailRenderer.h"
#include "WorldWorkspace.h"

#include <yave/assets/FolderAssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/DebugValues.h>

#include <y/utils/log.h>
#include <y/test/test.h>

#include <y/utils/log.h>
#include <y/utils/format.h>


namespace editor {

#ifdef Y_DEBUG
editor_action_desc("Debug assert", "Calls assert(false) and crashes the program", [] { y_debug_assert(false); })
#endif

editor_action("Quit", [] { imgui_platform()->main_window()->close(); })
editor_action("Show ImGui demo", [] { imgui_platform()->show_demo(); })
editor_action("Restore default layout", [] { ui().restore_default_layout(); })

editor_action_desc("Lag", "Pause execution for 1s to simulate load", [] { core::Duration::sleep(core::Duration::seconds(1)); })

editor_action_shortcut(ICON_FA_SAVE " Save", Key::Ctrl + Key::S, [] { save_world(); }, "File")
editor_action(ICON_FA_FOLDER " Load", [] { load_world(); }, "File")
editor_action_shortcut("New", Key::Ctrl + Key::N, [] { new_world(); }, "File")



namespace application {
std::unique_ptr<EditorResources> resources;
std::shared_ptr<AssetStore> asset_store;
std::unique_ptr<AssetLoader> loader;
std::unique_ptr<ThumbnailRenderer> thumbnail_renderer;
std::unique_ptr<UiManager> ui;
Workspace* workspace = nullptr;

std::unique_ptr<concurrent::JobSystem> editor_job_system;

ImGuiPlatform* imgui_platform = nullptr;

Settings settings;
}







void init_editor(ImGuiPlatform* platform, const Settings& settings) {
    application::settings = settings;
    application::imgui_platform = platform;
    application::editor_job_system = std::make_unique<concurrent::JobSystem>();

    const auto& store_dir = app_settings().editor.asset_store;

    application::resources = std::make_unique<EditorResources>();
    application::ui = std::make_unique<UiManager>();
    application::asset_store = std::make_shared<FolderAssetStore>(store_dir);
    application::loader = std::make_unique<AssetLoader>(application::asset_store, AssetLoadingFlags::SkipFailedDependenciesBit, 4);
    application::thumbnail_renderer = std::make_unique<ThumbnailRenderer>(*application::loader);
}


void destroy_editor() {
    application::ui = nullptr;
    application::workspace = nullptr;
    application::editor_job_system = nullptr; // finish thumbnail jobs before destroying their targets
    application::thumbnail_renderer = nullptr;
    application::loader = nullptr;
    application::asset_store = nullptr;
    application::resources = nullptr;
}

void run_editor() {
    application::imgui_platform->exec([] {
        if(application::workspace) {
            world_workspace().update();
        }
        application::ui->on_gui();
        if(application::workspace) {
            world_workspace().post_update();
        }
    });
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

ThumbnailRenderer& thumbnail_renderer() {
    return *application::thumbnail_renderer;
}

concurrent::JobSystem& world_job_system() {
    return world_workspace().job_system();
}

concurrent::JobSystem& editor_job_system() {
    return *application::editor_job_system;
}

const EditorResources& resources() {
    return *application::resources;
}

Workspace& current_workspace() {
    y_debug_assert(application::workspace);
    return *application::workspace;
}

Workspace* current_workspace_ptr() {
    return application::workspace;
}

WorldWorkspace& world_workspace() {
    WorldWorkspace* ws = dynamic_cast<WorldWorkspace*>(application::workspace);
    y_debug_assert(ws);
    return *ws;
}

void set_current_workspace(Workspace* workspace) {
    application::workspace = workspace;
}

void save_world() {
    world_workspace().save_world();
}

void load_world() {
    world_workspace().load_world();
}

void new_world() {
    world_workspace().new_world();
}

EditorWorld& current_world() {
    return world_workspace().world();
}

const Scene& current_scene() {
    return world_workspace().scene();
}

void set_scene_view(SceneView* scene) {
    world_workspace().set_scene_view(scene);
}

void unset_scene_view(SceneView* scene) {
    world_workspace().unset_scene_view(scene);
}

const SceneView& scene_view() {
    return world_workspace().scene_view();
}





DebugValues& debug_values() {
    static DebugValues values = {};
    return values;
}

DirectDraw& debug_drawer() {
    return world_workspace().debug_drawer();
}





Widget* focussed_widget() {
    return ui().focussed_widget();
}

Widget* last_focussed_widget() {
    return ui().last_focussed_widget();
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

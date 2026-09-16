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

#include <editor/WorldWorkspace.h>

#include <yave/assets/FolderAssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/DebugValues.h>
#include <yave/utils/DirectDraw.h>

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
editor_action("Close all", [] { ui().close_all(); })

editor_action_desc("Lag", "Pause execution for 1s to simulate load", [] { core::Duration::sleep(core::Duration::seconds(1)); })

editor_action_shortcut(ICON_FA_SAVE " Save", Key::Ctrl + Key::S, [](Workspace* ws) { ws->save(); }, "File")
editor_action(ICON_FA_FOLDER " Load", [](Workspace* ws) { ws->load(); }, "File")



namespace application {
std::unique_ptr<EditorResources> resources;
std::shared_ptr<AssetStore> asset_store;
std::unique_ptr<AssetLoader> loader;
std::unique_ptr<ThumbnailRenderer> thumbnail_renderer;
std::unique_ptr<UiManager> ui;
std::unique_ptr<DirectDraw> debug_drawer;

std::unique_ptr<concurrent::JobSystem> editor_job_system;

ImGuiPlatform* imgui_platform = nullptr;
Workspace* workspace = nullptr;

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
    application::debug_drawer = std::make_unique<DirectDraw>();
}


void destroy_editor() {
    application::debug_drawer = nullptr;
    application::ui = nullptr;
    application::editor_job_system = nullptr; // finish thumbnail jobs before destroying their targets
    application::thumbnail_renderer = nullptr;
    application::loader = nullptr;
    application::asset_store = nullptr;
    application::resources = nullptr;
}

void run_editor() {
    application::imgui_platform->exec([] {
        for(const auto& workspace : application::ui->workspaces()) {
            workspace->update();
        }

        application::ui->on_gui();

        for(const auto& workspace : application::ui->workspaces()) {
            workspace->post_update();
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

concurrent::JobSystem& editor_job_system() {
    return *application::editor_job_system;
}

const EditorResources& resources() {
    return *application::resources;
}

DebugValues& debug_values() {
    static DebugValues values = {};
    return values;
}

DirectDraw& debug_drawer() {
    WorldWorkspace* workspace = dynamic_cast<WorldWorkspace*>(application::workspace);
    return workspace ? workspace->debug_drawer() : *application::debug_drawer.get();
}




Workspace* current_workspace() {
    return application::workspace;
}

void set_current_workspace(Workspace* workspace) {
    application::workspace = workspace;
}

void unset_current_workspace(Workspace* workspace) {
    if(application::workspace == workspace) {
        application::workspace = nullptr;
    }
}





Widget* last_focussed_widget() {
    return ui().last_focussed_widget();
}

Widget* add_top_level_widget(std::unique_ptr<Widget> widget) {
    return ui().add_top_level_widget(std::move(widget));
}

Workspace* add_workspace(std::unique_ptr<Workspace> workspace) {
    return ui().add_workspace(std::move(workspace));
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

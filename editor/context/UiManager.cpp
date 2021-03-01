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

#include "UiManager.h"

#include <editor/context/EditorContext.h>
#include <editor/widgets/EntityView.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/properties/PropertyPanel.h>
#include <editor/widgets/MaterialEditor.h>
#include <editor/EngineView.h>
#include <editor/ui/Widget.h>

#include <editor/ui/ImGuiRenderer.h>
#include <external/imgui/yave_imgui.h>

#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/swapchain/FrameToken.h>

#include <y/io2/File.h>

#ifdef Y_OS_WIN
#include <windows.h>
#endif


#include <editor/widgets/EntityView.h>
#include <editor/widgets/FileBrowser.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/MemoryInfo.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/widgets/MaterialEditor.h>
#include <editor/widgets/AssetStringifier.h>
#include <editor/properties/PropertyPanel.h>
#include <editor/EngineView.h>

namespace editor {

UiManager::UiManager(ContextPtr ctx) : ContextLinked(ctx) {
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = "editor.ini";
    ImGui::GetIO().LogFilename = "editor_logs.txt";
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigDockingWithShift = false;
    //ImGui::GetIO().ConfigResizeWindowsFromEdges = true;

    if(io2::File::open("../editor.ini").is_ok()) {
        ImGui::GetIO().IniFilename = "../editor.ini";
    }

    show<EngineView>();
    show<EntityView>();
    show<ResourceBrowser>();
    show<PropertyPanel>();
    show<MaterialEditor>();

    _renderer = std::make_unique<ImGuiRenderer>(context());
}

UiManager::~UiManager() {
    ImGui::DestroyContext();
}

const ImGuiRenderer& UiManager::renderer() const {
    return *_renderer;
}

core::Span<std::unique_ptr<Widget>> UiManager::widgets() const {
    return _widgets;
}

bool UiManager::confirm(const char* message) {
    y_profile();
#ifdef Y_OS_WIN
    return MessageBox(GetActiveWindow(), message, "Confirm", MB_OKCANCEL) != IDCANCEL;
#else
#warning not supported
    return true;
#endif
}

void UiManager::ok(const char* title, const char* message) {
    y_profile();
#ifdef Y_OS_WIN
    MessageBox(GetActiveWindow(), message, title, MB_OK);
#else
#warning not supported
#endif
}

void UiManager::refresh_all() {
    y_profile();
    for(const auto& wid : _widgets) {
        wid->refresh();
    }
}

UiManager::Ids& UiManager::ids_for(Widget* elem) {
    return _ids[typeid(*elem)];
}

void UiManager::set_id(Widget* elem) {
    auto& ids = ids_for(elem);
    if(!ids.released.is_empty()) {
        elem->set_id(ids.released.pop());
    } else {
        elem->set_id(ids.next++);
    }
}


void UiManager::paint(CmdBufferRecorder& recorder, const FrameToken& token) {
    y_profile();

    ImGui::GetIO().DeltaTime = float(_frame_timer.reset().to_secs());
    ImGui::GetIO().DisplaySize = token.image_view.size();

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking |
                             ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoNavFocus;


    {
        y_profile_zone("begin frame");

        ImGui::NewFrame();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar | flags);

        ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        ImGui::PopStyleVar(3);
    }

    paint_menu_bar();
    paint_widgets(recorder);

    {
        y_profile_zone("end frame");
        ImGui::End();
        ImGui::Render();
    }

    Y_TODO(move that elsewhere)
    {
        y_profile_zone("render");
        Framebuffer framebuffer(token.image_view.device(), {token.image_view});
        RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
        _renderer->render(pass);
    }
}

static void fix_undocked_bg() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    const math::Vec4 window_bg = colors[ImGuiCol_WindowBg];
    const math::Vec4 child_col = colors[ImGuiCol_ChildBg];
    const math::Vec4 final(math::lerp(window_bg.to<3>(), child_col.to<3>(), child_col.w()), window_bg.w());
    ImGui::PushStyleColor(ImGuiCol_WindowBg, final);
}

void UiManager::paint_widget(Widget* widget, CmdBufferRecorder& recorder) {
    if(!widget->is_visible()) {
        return;
    }

    y_profile_zone(widget->_title_with_id.data());

    widget->before_paint();

    {
        ImGui::SetNextWindowSize(ImVec2(480, 480), ImGuiCond_FirstUseEver);

        // change windows bg to match docked (ie: inside frame) look
        if(!widget->_docked) {
            fix_undocked_bg();
        }

        const bool closable = widget->_closable && !widget->_has_children;
        const u32 extra_flags = 0;

        const bool b = ImGui::Begin(widget->_title_with_id.begin(), closable ? &widget->_visible : nullptr, widget->_flags | extra_flags);
        if(!widget->_docked) {
            ImGui::PopStyleColor();
        }

        widget->update_attribs();
        if(b) {
            widget->paint(recorder);
        }
        ImGui::End();
    }

    widget->after_paint();
}

void UiManager::cull_closed() {
    y_profile();

    for(const auto& wid : _widgets) {
        wid->_has_children = false;
    }

    for(const auto& wid : _widgets) {
        if(wid->_parent) {
            wid->_parent->_has_children = true;
        }
    }

    for(usize i = 0; i < _widgets.size(); ++i) {
        Widget* wid = _widgets[i].get();
        if(wid->_visible || wid->_has_children) {
            continue;
        }

        ids_for(_widgets[i].get()).released << _widgets[i]->_id;
        _widgets.erase_unordered(_widgets.begin() + i);
        --i;
    }
}


void UiManager::paint_widgets(CmdBufferRecorder& recorder) {
    y_profile();

    ImGui::ShowDemoWindow();

    bool refresh = false;
    for(const auto& wid : _widgets) {
        paint_widget(wid.get(), recorder);
        refresh |= wid->_refresh_all;
        wid->_refresh_all = false;
    }

    if(refresh) {
        refresh_all();
    }

    cull_closed();
}

void UiManager::paint_menu_bar() {
    if(ImGui::BeginMenuBar()) {
        if(ImGui::BeginMenu(ICON_FA_FILE " File")) {

            if(ImGui::MenuItem(ICON_FA_FILE " New")) {
                context()->new_world();
            }

            ImGui::Separator();

            if(ImGui::MenuItem(ICON_FA_SAVE " Save")) {
                context()->defer([ctx = context()] { ctx->save_world(); });
            }

            if(ImGui::MenuItem(ICON_FA_FOLDER " Load")) {
                context()->load_world();
            }

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View")) {
            if(ImGui::MenuItem("Engine view")) add_widget<EngineView>();
            if(ImGui::MenuItem("Entity view")) add_widget<EntityView>();
            if(ImGui::MenuItem("Resource browser")) add_widget<ResourceBrowser>();
            if(ImGui::MenuItem("Material editor")) add_widget<MaterialEditor>();

            ImGui::Separator();

            if(ImGui::BeginMenu("Debug")) {
                if(ImGui::MenuItem("Camera debug")) add_widget<CameraDebug>();

                ImGui::Separator();
                if(ImGui::MenuItem("Asset stringifier")) add_widget<AssetStringifier>();

                ImGui::Separator();
                if(ImGui::MenuItem("Flush reload")) context()->flush_reload();

                y_debug_assert(!(ImGui::Separator(), ImGui::MenuItem("Debug assert")));

                ImGui::EndMenu();
            }
            if(ImGui::BeginMenu("Statistics")) {
                if(ImGui::MenuItem("Performances")) add_widget<PerformanceMetrics>();
                if(ImGui::MenuItem("Memory info")) add_widget<MemoryInfo>();
                ImGui::EndMenu();
            }

            ImGui::Separator();

            if(ImGui::MenuItem(ICON_FA_COG " Settings")) add_widget<SettingsPanel>();

            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("Tools")) {
            if(ImGui::MenuItem("Reload resources")) context()->reload_device_resources();
            ImGui::EndMenu();
        }

#ifdef YAVE_PERF_LOG_ENABLED
        if(perf::is_capturing()) {
            ImGui::PushStyleColor(ImGuiCol_Text, 0xFF0000FF);
            if(ImGui::MenuItem(ICON_FA_STOPWATCH)) context()->end_perf_capture();
            ImGui::PopStyleColor();
        } else {
            if(ImGui::MenuItem(ICON_FA_STOPWATCH)) context()->start_perf_capture();
        }
#endif

        {
            const usize progress_bar_size = 300;
            //ImGui::SameLine(ImGui::GetWindowSize().x - progress_bar_size);
            auto progress_items = context()->notifications().progress_items();
            for(const auto& item : progress_items) {
                if(ImGui::GetContentRegionAvail().x < progress_bar_size) {
                    break;
                }
                if(!item.is_over()) {
                    ImGui::ProgressBar(item.fraction(), ImVec2(progress_bar_size, 0.0f), fmt_c_str("%: %/%", item.msg, item.it, item.size));
                }
            }
        }

        ImGui::EndMenuBar();
    }
}

}


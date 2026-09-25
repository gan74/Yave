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

#include "UiManager.h"

#include <editor/Settings.h>
#include <editor/utils/ui.h>
#include <editor/utils/StringMatcher.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/DebugValueEditor.h>
#include <editor/WorldWorkspace.h>
#include <editor/MaterialWorkspace.h>
#include <editor/BlueprintWorkspace.h>

#include <yave/graphics/device/Instance.h>

#include <yave/assets/AssetLoader.h>

#include <y/utils/format.h>

#include <algorithm>
#include <tuple>


#include <external/imgui/imgui_internal.h>


namespace editor {

editor_action("New empty workspace", [] { add_workspace(std::make_unique<EmptyWorkspace>()); })
editor_action("New world workspace", [] { add_workspace(std::make_unique<WorldWorkspace>()); })
editor_action("New material workspace", [] { add_workspace(std::make_unique<MaterialWorkspace>()); })
editor_action("New blueprint workspace", [] { add_workspace(std::make_unique<BlueprintWorkspace>()); })





static core::String shortcut_text(KeyCombination shortcut) {
    core::String text;
    if(!shortcut.is_empty()) {
        for(const Key k : all_keys()) {
            if(shortcut.contains(k)) {
                if(!text.is_empty()) {
                    text.push_back('+');
                }
                text += key_name(k);
            }
        }
    }
    return text;
}



UiManager::UiManager() : _main_dock_id(generate_dock_id()) {
    for(const EditorAction* action = all_actions(); action; action = action->next) {
        _actions << action;
        if(!action->shortcut.is_empty()) {
            _shortcuts << std::make_pair(action, false);
        }
    }

    std::sort(_actions.begin(), _actions.end(), [](const EditorAction* a, const EditorAction* b) {
        return std::lexicographical_compare(b->menu.begin(), b->menu.end(), a->menu.begin(), a->menu.end());
    });
}

UiManager::~UiManager() {
    close_all();
}

void UiManager::draw_dockspaces() {
    y_profile();

    ImGuiWindowClass host_class;
    {
        host_class.ClassId = _main_dock_id;
        host_class.DockingAllowUnclassed = true;
    }

    auto draw_workspace = [&](Workspace* workspace, bool open_widgets) {
        const u32 id = workspace->workspace_id();

        ImGui::SetNextWindowClass(&host_class);
        ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

        bool open = true;
        const bool visible = ImGui::Begin(fmt_c_str("{}##workspace_{}", workspace->name(), id), &open);

        if(open_widgets) {
            create_workspace_widgets(workspace);
        }

        ImGuiWindowClass window_class;
        {
            window_class.ClassId = id;
            window_class.DockingAllowUnclassed = true;
        }

        ImGui::DockSpace(id, ImVec2(0.0f, 0.0f), visible ? ImGuiDockNodeFlags_None : ImGuiDockNodeFlags_KeepAliveOnly, &window_class);
        ImGui::End();

        return open;
    };

    for(const auto& workspace : _new_workspaces) {
        draw_workspace(workspace.get(), true);
    }

    for(usize i = 0; i != _workspaces.size(); ++i) {
        if(!draw_workspace(_workspaces[i].get(), false)) {
            _to_destroy.emplace_back(std::move(_workspaces[i]));
            _workspaces.erase_unordered(_workspaces.begin() + i);
            --i;
        }
    }
 
    {
        for(auto& workspace : _new_workspaces) {
            _workspaces.emplace_back(std::move(workspace));
        }
        _new_workspaces.clear();
    }
}

void UiManager::create_workspace_widgets(Workspace* workspace) {
    const u32 id = workspace->workspace_id();

    // Skip building layout if it already exists in the ini
    const bool build_layout = !ImGui::DockBuilderGetNode(id);

    ImGuiID left = 0;
    ImGuiID right = 0;
    ImGuiID center = id;

    if(build_layout) {
        ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(id, ImGui::GetContentRegionAvail());
    }

    auto create_docking_node = [&](DockingNode node) {
        switch(node) {
            case DockingNode::Left:
                if(!left) {
                    ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.25f, &left, &center);
                }
                return left;

            case DockingNode::Right:
                if(!right) {
                    ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, &right, &center);
                }
                return right;

            default:
                return center;
        }
    };

    for(const EditorWidgetDesc* desc = all_widget_descs(); desc; desc = desc->next) {
        if(desc->default_node != DockingNode::None) {
            if(std::unique_ptr<Widget> widget = desc->create(workspace)) {
                if(build_layout) {
                    widget->_dock_id = create_docking_node(desc->default_node);
                }
                add_widget(std::move(widget));
            }
        }
    }

    if(build_layout) {
        ImGui::DockBuilderFinish(id);
    }
}

void UiManager::process_new_widgets() {
    y_profile();

    for(auto& widget : _new_widgets) {
        _widgets.emplace_back(std::move(widget));
    }
    _new_widgets.clear();
}

void UiManager::process_deletions() {
    y_profile();

    for(const auto& workspace : _to_destroy) {
        for(const auto& widget : _widgets) {
            if(widget->belongs_to(workspace.get())) {
                widget->close();
            }
        }
    }

    bool has_modal = false;
    for(usize i = 0; i != _widgets.size(); ++i) {
        Widget* widget = _widgets[i].get();

        if(!widget->is_visible() && !widget->should_keep_alive()) {
            y_profile_dyn_zone(fmt_c_str("destroying '{}'", widget->_title_with_id));

            if(_focussed == widget) {
                _focussed = nullptr;
            }
            if(_last_focussed == widget) {
                _last_focussed = nullptr;
            }

            _widgets.erase_unordered(_widgets.begin() + i);
            --i;
        } else {
            has_modal |= widget->is_modal();
        }
    }

    if(!has_modal) {
        for(usize i = 0; i != _to_destroy.size(); ++i) {
            bool keep_alive = false;
            for(const auto& widget : _widgets) {
                keep_alive |= widget->belongs_to(_to_destroy[i].get());
            }
            if(!keep_alive) {
                log_msg(fmt("Closing workspace: '{}'", _to_destroy[i]->name()));
                ImGui::DockBuilderRemoveNode(_to_destroy[i]->workspace_id());
                unset_current_workspace(_to_destroy[i].get());
                _to_destroy.erase_unordered(_to_destroy.begin() + i);
                --i;
            }
        }
    }
}

void UiManager::on_gui() {
    y_profile();

    {
        ImGuiWindowClass main_class;
        {
            main_class.ClassId = _main_dock_id;
            main_class.DockingAllowUnclassed = true;
        }

        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImGui::GetStyleColorVec4(ImGuiCol_ModalWindowDimBg));
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_AutoHideTabBar, &main_class);
        ImGui::PopStyleColor();
    }

    update_fps_counter();
    update_shortcuts();
    draw_menu_bar();

    draw_dockspaces();

    process_new_widgets();
    
    if(_workspaces.is_empty()) {
        ImGui::OpenPopup("##noworkspace");
        
        const ImGuiWindowFlags popup_flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_AlwaysAutoResize
        ;
        if(ImGui::BeginPopupModal("##noworkspace", nullptr, popup_flags)) {
            if(ImGui::Button("New world workspace")) {
                add_workspace(std::make_unique<WorldWorkspace>());
            }
            if(ImGui::Button("New empty workspace")) {
                add_workspace(std::make_unique<EmptyWorkspace>());
            }
            ImGui::EndPopup();
        }
    }


    {
        Widget* focussed = nullptr;
        for(const auto& widget : _widgets) {
            y_profile_dyn_zone(widget->_title_with_id.data());

            widget->draw(false);

            if(widget->_focussed) {
                focussed = widget.get();
            }
        }

        _focussed = focussed;
        _last_focussed = _focussed ? _focussed : _last_focussed;
    }

    process_deletions();
}

void UiManager::update_fps_counter() {
    float& current_frame = _frame_times[_frame_number++ % _frame_times.size()];
    _total_time -= current_frame;
    current_frame = float(_timer.reset().to_millis());
    _total_time += current_frame;
}

void UiManager::draw_fps_counter() {
    const float avg_time = _total_time / std::min(u64(_frame_times.size()), _frame_number);
    if(ImGui::MenuItem(fmt_c_str("FPS: {:.1f} {:.01f} ms", 1000.0f / avg_time, avg_time))) {
        add_widget(std::make_unique<PerformanceMetrics>());
    }
}

void UiManager::update_shortcuts() {
    y_profile();

    const auto& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard) {
        return;
    }

    KeyCombination keys;
    for(const Key k : all_keys()) {
        if(ImGui::IsKeyDown(to_imgui_key(k))) {
            keys += k;
        }
    }

    {
        Workspace* workspace = current_workspace();
        for(auto&& action : _shortcuts) {
            if(keys == action.first->shortcut) {
                if(!action.second) {
                    const EditorAction* a = action.first;
                    if(!a->enabled || a->enabled(workspace)) {
                        a->function(workspace);
                    }
                    action.second = true;
                }
            } else {
                action.second = false;
            }
        }
    }
}

void UiManager::draw_menu_bar() {
    Workspace* workspace = current_workspace();

    ImGui::PushID("##mainmenubar");
    if(ImGui::BeginMainMenuBar()) {
        if(ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }

        if(ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
        }

        if(asset_loader().is_loading()) {
            ImGui::Separator();
            ImGui::TextColored(imgui::error_text_color, ICON_FA_DATABASE);
            if(ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Assets are loading%s", imgui::ellipsis());
                ImGui::EndTooltip();
            }
        }

        if(app_settings().ui.draw_fps_counter) {
            ImGui::Separator();
            draw_fps_counter();
        }

        if(workspace) {
            ImGui::TextUnformatted(fmt_c_str("{}: {}", workspace->name(), static_cast<const void*>(workspace)));
        }

        if(ImGui::GetIO().WantCaptureKeyboard) {
            ImGui::TextUnformatted(ICON_FA_KEYBOARD);
        }

        if(instance_params().validation_layers) {
            ImGui::TextColored(imgui::error_text_color, "(Debug layers enabled)");
        }

        if(ImGui::MenuItem(ICON_FA_BUG)) {
            add_widget(std::make_unique<DebugValueEditor>());
        }


        for(const EditorAction* action : _actions) {
            if(!action->menu.size()) {
                continue;
            }

            usize stack_size = 0;
            for(std::string_view menu : action->menu) {
                if(!ImGui::BeginMenu(menu.data())) {
                    break;
                }
                ++stack_size;
            }

            if(stack_size == action->menu.size()) {
                const core::String shortcut = shortcut_text(action->shortcut);
                const bool enabled = !action->enabled || action->enabled(workspace);
                if(ImGui::MenuItem(action->name.data(), shortcut.is_empty() ? nullptr : shortcut.data(), false, enabled)) {
                    action->function(workspace);
                }
            }

            for(usize i = 0; i != stack_size; ++i) {
                ImGui::EndMenu();
            }
        }

        {
            const float search_bar_size = 250.0;
            const float margin = ImGui::CalcTextSize(ICON_FA_SEARCH " ").x;
            const float max_x = ImGui::GetContentRegionAvail().x + ImGui::GetCursorScreenPos().x - ImGui::GetWindowPos().x;
            const float offset = max_x - (search_bar_size + margin);

            if(offset > 0.0f) {
                ImGui::Indent(offset);
                ImGui::SetNextItemWidth(-margin);

                imgui::search_bar(ICON_FA_SEARCH "##searchbar", _search_pattern.data(), _search_pattern.size());

                if(imgui::begin_suggestion_popup()) {
                    StringMatcher matcher(_search_pattern.data());
                    for(const EditorAction* action : _actions) {
                        if((action->flags & EditorAction::Contextual) == EditorAction::Contextual) {
                            continue;
                        }

                        if(action->enabled && !(action->enabled(workspace))) {
                            continue;
                        }

                        if(matcher.matches(action->name)) {
                            const core::String shortcut = shortcut_text(action->shortcut);
                            if(imgui::suggestion_item(action->name.data(), shortcut.is_empty() ? nullptr : shortcut.data())) {
                                action->function(workspace);
                                _search_pattern[0] = 0;
                            }
                            if(!action->description.empty() && ImGui::IsItemHovered()) {
                                ImGui::BeginTooltip();
                                ImGui::TextUnformatted(action->description.data());
                                ImGui::EndTooltip();
                            }
                        }
                    }


                    imgui::end_suggestion_popup();
                }
            }
        }

        ImGui::EndMainMenuBar();
    }
    ImGui::PopID();
}

Widget* UiManager::add_widget(std::unique_ptr<Widget> widget) {
    if(!widget) {
        return nullptr;
    }
    return _new_widgets.emplace_back(std::move(widget)).get();
}

Workspace* UiManager::add_workspace(std::unique_ptr<Workspace> workspace) {
    return _new_workspaces.emplace_back(std::move(workspace)).get();
}

void UiManager::close_all() {
    _widgets.clear();
    _workspaces.clear();
    _to_destroy.clear();
    _new_widgets.clear();
    _new_workspaces.clear();
    _focussed = nullptr;
    _last_focussed = nullptr;
    set_current_workspace(nullptr);
}

core::Span<std::unique_ptr<Widget>> UiManager::top_level_widgets() const {
    return _widgets;
}

core::Span<std::unique_ptr<Workspace>> UiManager::workspaces() const {
    return _workspaces;
}

Widget* UiManager::last_focussed_widget() {
    return _last_focussed;
}

u32 UiManager::generate_dock_id() {
    // Avoid collision with imgui's own ids
    return 0xF0000000u + (++_dock_id);
}

u32 UiManager::main_dock_id() const {
    return _main_dock_id;
}

}

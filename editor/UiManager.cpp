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
#include <editor/widgets/WorkArea.h>
#include <editor/WorldWorkspace.h>

#include <yave/graphics/device/Instance.h>

#include <yave/assets/AssetLoader.h>

#include <algorithm>
#include <tuple>

namespace editor {

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




UiManager::UiManager() {
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
}




void UiManager::on_gui() {
    y_profile();


    update_fps_counter();
    update_shortcuts();
    draw_menu_bar();


    Widget* focussed = nullptr;
    for(usize i = 0; i != _widgets.size(); ++i) {
        Widget* widget = _widgets[i].get();
        y_profile_dyn_zone(widget->_title_with_id.data());

        widget->draw(false);

        if(Widget* f = widget->find_focussed()) {
            focussed = f;
        }
    }

    _focussed = focussed;
    _last_focussed = _focussed ? _focussed : _last_focussed;

    for(usize i = 0; i != _widgets.size(); ++i) {
        Widget* widget = _widgets[i].get();
        if(!widget->is_visible() && !widget->has_keep_alive()) {
            y_profile_dyn_zone(fmt_c_str("destroying '{}'", widget->_title_with_id));

            for(Widget* w = _focussed; w; w = w->_parent) {
                if(w == widget) {
                    _focussed = nullptr;
                    break;
                }
            }
            for(Widget* w = _last_focussed; w; w = w->_parent) {
                if(w == widget) {
                    _last_focussed = nullptr;
                    break;
                }
            }

            _widgets.erase_unordered(_widgets.begin() + i);
            --i;
        }
    }
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
        add_top_level_widget(std::make_unique<PerformanceMetrics>());
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
                    action.first->function(workspace);
                    action.second = true;
                }
            } else {
                action.second = false;
            }
        }
    }
}

void UiManager::draw_menu_bar() {
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

        if(ImGui::GetIO().WantCaptureKeyboard) {
            ImGui::TextUnformatted(ICON_FA_KEYBOARD);
        }

        if(instance_params().validation_layers) {
            ImGui::TextColored(imgui::error_text_color, "(Debug layers enabled)");
        }

        if(ImGui::MenuItem(ICON_FA_BUG)) {
            add_top_level_widget(std::make_unique<DebugValueEditor>());
        }

        Workspace* workspace = current_workspace();

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
                if(ImGui::MenuItem(action->name.data(), shortcut.is_empty() ? nullptr : shortcut.data())) {
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

Widget* UiManager::add_top_level_widget(std::unique_ptr<Widget> widget) {
    Widget* wid = widget.get();
    _widgets << std::move(widget);
    return wid;
}

void UiManager::close_all() {
    _widgets.clear();
    _focussed = nullptr;
    _last_focussed = nullptr;
}

core::Span<std::unique_ptr<Widget>> UiManager::widgets() const {
    return _widgets;
}

Widget* UiManager::last_focussed_widget() {
    return _last_focussed;
}

}


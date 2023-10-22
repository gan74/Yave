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

#include "UiManager.h"

#include <editor/Settings.h>
#include <editor/utils/ui.h>
#include <editor/widgets/PerformanceMetrics.h>

#include <yave/assets/AssetLoader.h>
#include <y/core/HashMap.h>

#include <regex>
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

    if(!_frame_number) {
        open_default_widgets();
    }

    update_fps_counter();
    update_shortcuts();
    draw_menu_bar();

    core::FlatHashMap<Widget*, int> to_destroy;

    for(auto& widget : _widgets) {
        y_profile_dyn_zone(widget->_title_with_id.data());

        _auto_parent = widget.get();
        widget->draw(false);

        if(!widget->is_visible() && !widget->should_keep_alive()) {
            to_destroy[widget.get()];
        }
    }

    _auto_parent = nullptr;

   if(!to_destroy.is_empty()) {
        for(usize i = 0;  i != _widgets.size(); ++i) {
            Widget* wid = _widgets[i].get();
            bool destroy = to_destroy.contains(wid);
            for(Widget* parent = wid->_parent; parent && !destroy; parent = parent->_parent) {
                destroy |= to_destroy.contains(parent);
            }

            if(destroy) {
                _ids[typeid(*wid)].released << wid->_id;
                _widgets.erase_unordered(_widgets.begin() + i);
                --i;
            }
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

    for(auto&& action : _shortcuts) {
        if(keys.contains(action.first->shortcut)) {
            if(!action.second) {
                action.first->function();
                action.second = true;
            }
        } else {
            action.second = false;
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
                    action->function();
                }
            }

            for(usize i = 0; i != stack_size; ++i) {
                ImGui::EndMenu();
            }
        }

        {
            const float search_bar_size = 200.0;
            const float margin = ImGui::CalcTextSize(ICON_FA_SEARCH " ").x;
            const float offset = ImGui::GetContentRegionMax().x - (search_bar_size + margin);

            if(offset > 0.0f) {
                ImGui::Indent(offset);
                ImGui::SetNextItemWidth(-margin);

                imgui::search_bar(ICON_FA_SEARCH "##searchbar", _search_pattern.data(), _search_pattern.size());

                if(imgui::begin_suggestion_popup()) {
                    const std::regex regex(_search_pattern.data(), std::regex::icase);
                    for(const EditorAction* action : _actions) {
                        if(std::regex_search(action->name.data(), regex)) {
                            const core::String shortcut = shortcut_text(action->shortcut);
                            if(imgui::suggestion_item(action->name.data(), shortcut.is_empty() ? nullptr : shortcut.data())) {
                                action->function();
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

Widget* UiManager::add_widget(std::unique_ptr<Widget> widget, bool auto_parent) {
    Widget* wid = widget.get();

    if(auto_parent && _auto_parent) {
        wid->set_parent(_auto_parent);
    }

    set_widget_id(wid);
    _widgets << std::move(widget);

    return wid;
}

void UiManager::restore_default_layout() {
    close_all();
    open_default_widgets();
}

void UiManager::open_default_widgets() {
    for(const EditorWidget* widget = all_widgets(); widget; widget = widget->next) {
        if(widget->open_on_startup) {
            widget->create();
        }
    }
}

void UiManager::close_all() {
    _widgets.clear();
    _ids.clear();
}

core::Span<std::unique_ptr<Widget>> UiManager::widgets() const {
    return _widgets;
}

void UiManager::set_widget_id(Widget* widget) {
    WidgetIdStack& ids = _ids[typeid(*widget)];
    if(!ids.released.is_empty()) {
        widget->set_id(ids.released.pop());
    } else {
        widget->set_id(++ids.next);
    }
}

}


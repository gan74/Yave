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

#include <editor/utils/ui.h>
#include <editor/widgets/EngineView.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/widgets/FileBrowser.h>

#include <yave/utils/FileSystemModel.h>

#include <y/core/HashMap.h>

#include <external/imgui/yave_imgui.h>

#include <regex>
#include <tuple>

namespace editor {


UiManager::UiManager() {
    for(const EditorAction* action = all_actions(); action; action = action->next) {
        _actions << action;
    }

    std::sort(_actions.begin(), _actions.end(), [](const EditorAction* a, const EditorAction* b) {
        return std::lexicographical_compare(b->menu.begin(), b->menu.end(), a->menu.begin(), a->menu.end());
    });
}

UiManager::~UiManager() {
}

void UiManager::on_gui() {
    draw_menu_bar();

    core::ExternalHashMap<Widget*, int> to_destroy;

    for(auto& widget : _widgets) {
        y_profile_dyn_zone(widget->_title_with_id.data());

        _auto_parent = widget.get();
        widget->draw(false);

        if(!widget->is_visible()) {
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

void UiManager::draw_menu_bar() {
    if(ImGui::BeginMainMenuBar()) {

        if(ImGui::BeginMenu("File")) {
            ImGui::EndMenu();
        }
        if(ImGui::BeginMenu("View")) {
            ImGui::EndMenu();
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
                if(ImGui::MenuItem(action->name.data())) {
                    action->function();
                }
            }

            for(usize i = 0; i != stack_size; ++i) {
                ImGui::EndMenu();
            }
        }

        {
            const float margin = 24.0f;
            const float search_bar_size = 200.0;
            const float offset = ImGui::GetContentRegionMax().x - (search_bar_size + margin);

            if(offset > 0.0f) {
                ImGui::Indent(offset);
                ImGui::SetNextItemWidth(-margin);

                imgui::search_bar(_search_pattern.data(), _search_pattern.size());

                if(imgui::begin_suggestion_popup()) {
                    const std::regex regex(_search_pattern.data(), std::regex::icase);
                    for(const EditorAction* action : _actions) {
                        if(std::regex_search(action->name.data(), regex)) {
                            if(imgui::suggestion_item(action->name.data())) {
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

void UiManager::close_all() {
    _widgets.clear();
    _ids.clear();
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


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

#include "Widget.h"
#include "UiManager.h"

#include <editor/utils/ui.h>

#include <y/utils/format.h>

namespace editor {

namespace detail {
EditorWidget* first_widget = nullptr;
void register_widget(EditorWidget* widget) {
    log_msg(fmt("Registering widget \"{}\"", widget->name), Log::Debug);
    widget->next = first_widget;
    first_widget = widget;
}
}

const EditorWidget* all_widgets() {
    return detail::first_widget;
}


static u64 next_widget_id() {
    static u64 id = 0;
    return ++id;
}

Widget::Widget(std::string_view title, int flags) : _id(next_widget_id()), _flags(flags) {
    set_title(title);
}

Widget::~Widget() {
}

void Widget::close() {
    _visible = false;
}

bool Widget::is_visible() const {
    return _visible;
}

void Widget::set_visible(bool visible) {
    _visible = visible;
}

void Widget::set_modal(bool modal) {
    _modal = modal;
}

Widget* Widget::add_child_widget(std::unique_ptr<Widget> child) {
    Widget* widget = child.get();

    y_debug_assert(widget);
    y_debug_assert(!widget->_parent);

    widget->_parent = this;
    _children << std::move(child);
    
    return widget;
}

void Widget::refresh() {
}

void Widget::refresh_all() {
    Y_TODO(fix refresh)
    refresh();
}

void Widget::on_gui() {
    ImGui::TextUnformatted("Empty widget");
}

void Widget::on_inactive_gui() {
}

bool Widget::before_gui() {
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, 0);

    return true;
}

void Widget::after_gui() {
    ImGui::PopStyleColor();
}

void Widget::prepare_window() {
}

void Widget::draw_gui_inside() {
    draw(true);
}

bool Widget::should_keep_alive() const {
    return false;
}

bool Widget::has_keep_alive() const {
    if(should_keep_alive()) {
        return true;
    }
    for(const auto& child : _children) {
        if(child->has_keep_alive()) {
            return true;
        }
    }
    return false;
}

Widget* Widget::find_focussed() {
    for(auto& child : _children) {
        if(Widget* focussed = child->find_focussed()) {
            return focussed;
        }
    }
    return _focussed ? this : nullptr;
}

void Widget::draw_children() {
    for(usize i = 0; i != _children.size(); ++i) {
        _children[i]->draw(false);
    }
    prune_children();
}

void Widget::prune_children() {
    UiManager& ui_manager = ui();
    for(usize i = 0; i != _children.size(); ++i) {
        Widget* child = _children[i].get();
        if(!child->is_visible() && !child->has_keep_alive()) {
            for(Widget* w = ui_manager._focussed; w; w = w->_parent) {
                if(w == child) {
                    ui_manager._focussed = nullptr;
                    break;
                }
            }
            for(Widget* w = ui_manager._last_focussed; w; w = w->_parent) {
                if(w == child) {
                    ui_manager._last_focussed = nullptr;
                    break;
                }
            }
            _children.erase_unordered(_children.begin() + i);
            --i;
        }
    }
}

void Widget::draw(bool inside) {
    if(!_visible || !before_gui()) {
        draw_children();
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

    const bool is_modal = _modal;

    bool opened = false;
    if(inside) {
        opened = ImGui::BeginChild(_title_with_id.data(), {}, false, _flags);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
        if(is_modal) {
            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetColorU32(ImGuiCol_ChildBg));
            if(_visible) {
                ImGui::OpenPopup(_title_with_id.data());
            }
            opened = ImGui::BeginPopupModal(_title_with_id.data(), &_visible, _flags);
        } else {
            prepare_window();
            opened = ImGui::Begin(_title_with_id.data(), &_visible, _flags);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    _focussed = opened && ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    if(opened) {
        on_gui();
    } else {
        on_inactive_gui();
    }

    if(inside) {
        ImGui::EndChild();
    } else if(is_modal) {
        if(opened) {
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor();
    } else {
        ImGui::End();
    }

    after_gui();

    draw_children();
}

math::Vec2ui Widget::content_size() const {
    return math::Vec2ui(to_y(ImGui::GetWindowSize())); //(math::Vec2(ImGui::GetWindowContentRegionMax()) - math::Vec2(ImGui::GetWindowContentRegionMin())).max(math::Vec2(1.0f));
}

void Widget::set_title(std::string_view title) {
    _title_with_id = fmt("{}##{}", title, _id);
}

}


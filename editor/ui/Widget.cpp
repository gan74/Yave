/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <imgui/yave_imgui.h>

namespace editor {

Widget::Widget(std::string_view title, u32 flags) :
        _flags(flags) {
    set_title(title);
}

Widget::~Widget() {
    y_always_assert(!_has_children, "Widget was closed while having children");
}

const math::Vec2& Widget::position() const {
    return _position;
}

const math::Vec2& Widget::size() const {
    return _size;
}

bool Widget::is_focussed() const {
    return _focussed;
}

bool Widget::is_mouse_inside() const {
    return _mouse_inside;
}

void Widget::set_closable(bool closable) {
    _closable = closable;
}

void Widget::set_flags(u32 flags) {
    _flags = flags;
}

bool Widget::can_destroy() const {
    return true;
}

math::Vec2ui Widget::content_size() const {
    return (math::Vec2(ImGui::GetWindowContentRegionMax()) - math::Vec2(ImGui::GetWindowContentRegionMin())).max(math::Vec2(1.0f));
}

void Widget::update_attribs() {
    _position = ImGui::GetWindowPos();
    _size = ImGui::GetWindowSize();
    _docked = ImGui::IsWindowDocked();
    _focussed = ImGui::IsWindowFocused();

    const math::Vec2 mouse_pos = math::Vec2(ImGui::GetIO().MousePos) - math::Vec2(ImGui::GetWindowPos());
    const auto less = [](const math::Vec2& a, const math::Vec2& b) { return a.x() < b.x() && a.y() < b.y(); };
    _mouse_inside = less(mouse_pos, ImGui::GetWindowContentRegionMax()) && less(ImGui::GetWindowContentRegionMin(), mouse_pos);
}

void Widget::set_id(u64 id) {
    _id = id;
    set_title(_title);
}

void Widget::set_title(std::string_view title) {
    const core::String new_title = fmt("%##%", title, _id);
    _title_with_id = std::move(new_title);
    _title = std::string_view(_title_with_id.begin(), title.size());
}

void Widget::refresh_all() {
    _refresh_all = true;
}

void Widget::refresh() {
}

void Widget::set_parent(Widget* parent) {
    y_always_assert(!_parent || !parent, "Widget already has a parent");
    _parent = parent;
}

bool Widget::has_parent() const {
    return _parent;
}

bool Widget::is_visible() const {
    return _visible;
}

std::string_view Widget::title() const {
    return _title;
}

void Widget::show() {
    _visible = true;
}

void Widget::close() {
    _visible = false;
}

UiManager* Widget::manager() const {
    y_debug_assert(_manager);
    return _manager;
}

}


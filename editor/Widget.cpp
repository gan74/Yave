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

#include "Widget.h"

#include <external/imgui/yave_imgui.h>

namespace editor {

Widget::Widget(std::string_view title, int flags) : _flags(flags) {
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

void Widget::set_parent(Widget* parent) {
    y_debug_assert(!parent != !_parent);
    _parent = parent;
}

void Widget::refresh() {
}

void Widget::refresh_all() {
    Y_TODO(fix refresh)
    refresh();
}

void Widget::on_gui() {
    ImGui::Text("Empty widget");
}

bool Widget::before_gui() {
    return true;
}

void Widget::after_gui() {
}

void Widget::draw_gui_inside() {
    draw(true);
}


void Widget::draw(bool inside) {
    if(!_visible || !before_gui()) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(520, 600), ImGuiCond_FirstUseEver);

    const bool opened = inside
        ? ImGui::BeginChild(_title_with_id.data(), math::Vec2(), false, _flags)
        : ImGui::Begin(_title_with_id.data(), &_visible, _flags);

    if(opened) {
        on_gui();
    }

    if(inside) {
        ImGui::EndChild();
    } else {
        ImGui::End();
    }

    after_gui();
}

math::Vec2ui Widget::content_size() const {
    return (math::Vec2(ImGui::GetWindowContentRegionMax()) - math::Vec2(ImGui::GetWindowContentRegionMin())).max(math::Vec2(1.0f));
}

void Widget::set_flags(int flags) {
    _flags |= flags;
}

void Widget::set_id(u64 id) {
    _id = id;
    set_title(_title);
}

void Widget::set_title(std::string_view title) {
    _title_with_id = fmt("%##%", title, _id);
    _title = std::string_view(_title_with_id.begin(), title.size());
}

}


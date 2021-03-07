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

Widget::Widget(std::string_view title) {
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

void Widget::refresh() {
}

void Widget::refresh_all() {
    refresh();
}

void Widget::draw_gui() {
    ImGui::Text("Empty widget");
}

void Widget::draw_gui_inside() {
    if(begin()) {
        draw_gui();
        end();
    }
}

bool Widget::begin() {
    return _visible && ImGui::Begin(_title_with_id.data(), &_visible);
}

void Widget::end() {
    ImGui::End();
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


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
        UiElement(title),
        _flags(flags) {
}

Widget::~Widget() {
    y_always_assert(!has_children(), "Widget was closed while having children");
}

const math::Vec2& Widget::position() const {
    y_debug_assert(!has_parent());
    return _position;
}

const math::Vec2& Widget::size() const {
    y_debug_assert(!has_parent());
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

static void fix_undocked_bg() {
    ImVec4* colors = ImGui::GetStyle().Colors;
    const math::Vec4 window_bg = colors[ImGuiCol_WindowBg];
    const math::Vec4 child_col = colors[ImGuiCol_ChildBg];
    const math::Vec4 final(math::lerp(window_bg.to<3>(), child_col.to<3>(), child_col.w()), window_bg.w());
    ImGui::PushStyleColor(ImGuiCol_WindowBg, final);
}

void Widget::paint(CmdBufferRecorder& recorder, const FrameToken& token) {
    if(!is_visible()) {
        return;
    }

    y_profile_zone(_title_with_id.data());

    before_paint();

    if(has_parent()) {
        y_always_assert(!has_children(), "Widgets drawn in parents can not have childrens");

        /*if(ImGui::BeginChild(_title_with_id.begin(), ImVec2(0, 0), false, _flags)) {
            paint_ui(recorder, token);
        }
        ImGui::EndChild();*/

        ImGui::BeginGroup();
        ImGui::PushID(_title_with_id.begin());
        {
            update_attribs();
            paint_ui(recorder, token);
        }
        ImGui::PopID();
        ImGui::EndGroup();

    } else {
        ImGui::SetNextWindowSize(ImVec2(480, 480), ImGuiCond_FirstUseEver);

        // change windows bg to match docked (ie: inside frame) look
        if(!_docked) {
            fix_undocked_bg();
        }

        const bool closable = _closable && !has_children();
        const u32 extra_flags = is_child() ? ImGuiWindowFlags_NoSavedSettings : 0;

        const bool b = ImGui::Begin(_title_with_id.begin(), closable ? &_visible : nullptr, _flags | extra_flags);
        if(!_docked) {
            ImGui::PopStyleColor();
        }

        update_attribs();
        if(b) {
            paint_ui(recorder, token);
        }
        ImGui::End();
    }

    after_paint();
}

}


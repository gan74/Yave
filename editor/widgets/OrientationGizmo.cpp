/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include "OrientationGizmo.h"

#include <editor/EditorWorld.h>

#include <yave/scene/SceneView.h>

#include <editor/utils/ui.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

static constexpr float orientation_gizmo_size = 45.0f;
static constexpr float orientation_gizmo_width = 2.5f;
static constexpr float orientation_gizmo_end_point_width = 8.0f;

static constexpr u32 orientation_gizmo_alpha = 0xB0000000;

OrientationGizmo::OrientationGizmo(SceneView* view) : _scene_view(view) {
}

bool OrientationGizmo::is_dragging() const {
    return _dragging;
}

void OrientationGizmo::draw() {
    y_profile();

    _dragging &= ImGui::IsMouseDown(0);

    const math::Vec2 offset = ImGui::GetWindowPos();
    const math::Vec2 viewport = ImGui::GetWindowSize();
    const math::Vec2 center = offset + viewport - orientation_gizmo_size;
    const auto view_proj = _scene_view->camera().viewproj_matrix();
    const auto view = _scene_view->camera().view_matrix();

    const float ratio = viewport.y() / viewport.x();

    std::array<i32, 4> axes = {-1, 0, 1, 2};
    std::sort(axes.begin(), axes.end(), [&](i32 a, i32 b) {
        math::Vec4 va, vb;
        if(a >= 0) {
            va[a] = 1.0f;
        }
        if(b >= 0) {
            vb[b] = 1.0f;
        }
        return (view * va).z() < (view * vb).z();
    });

    const char* axis_name[] = {"X", "Y", "Z"};

    for(i32 i : axes) {
        if(i < 0) {
            ImGui::GetWindowDrawList()->AddCircleFilled(center, orientation_gizmo_width, 0xFFFFFFFF);
            continue;
        }

        math::Vec4 v;
        v[i] = orientation_gizmo_size + orientation_gizmo_end_point_width;
        const auto h_pos = view_proj * v;
        const math::Vec2 axis = (h_pos.to<2>() * 0.5f + 0.5f) * math::Vec2(1.0f, ratio);
        const u32 color = orientation_gizmo_alpha | imgui::gizmo_color(i);
        const math::Vec2 end = center + axis;
        if(axis.length() > orientation_gizmo_end_point_width) {
            const math::Vec2 dir = axis.normalized();
            ImGui::GetWindowDrawList()->AddLine(center + dir * orientation_gizmo_width, end - dir * (orientation_gizmo_end_point_width - 1.0f), color, orientation_gizmo_width);
        }

        ImGui::GetWindowDrawList()->AddCircleFilled(end, orientation_gizmo_end_point_width, color);

        // const ImFontGlyph* glyph = ImGui::GetFont()->FindGlyph(axis_name[i][0]);
        const math::Vec2 text_offset = math::Vec2(ImGui::CalcTextSize(axis_name[i]));
        ImGui::GetWindowDrawList()->AddText(end - text_offset * 0.5f, orientation_gizmo_alpha, axis_name[i]);
    }

    const float gizmo_radius = orientation_gizmo_size - orientation_gizmo_end_point_width * 0.5f;
    if(_dragging || (center - math::Vec2(ImGui::GetMousePos())).length2() < gizmo_radius * gizmo_radius) {
        ImGui::GetWindowDrawList()->AddCircle(center, gizmo_radius, orientation_gizmo_alpha | 0x00FFFFFF);
        if(ImGui::IsMouseClicked(0)) {
            _dragging = true;
        }
    }
}

}

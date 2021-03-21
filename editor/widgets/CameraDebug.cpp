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
#include "CameraDebug.h"

#include <yave/scene/SceneView.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

CameraDebug::CameraDebug() : Widget(ICON_FA_VIDEO " Camera debug", ImGuiWindowFlags_AlwaysAutoResize) {
}

void CameraDebug::on_gui() {
    const auto& camera = scene_view().camera();
    auto pos = camera.position();
    auto fwd = camera.forward();
    auto rht = camera.right();
    const auto up = fwd.cross(rht);

    const math::Quaternion<> rot = math::Quaternion<>::from_base(fwd, rht, up);


    ImGui::Text("FoV: %.1f", camera.field_of_view());
    ImGui::Text("Aspect ratio: %.2f", camera.aspect_ratio());

    ImGui::Separator();

    ImGui::Text("position: %.1f, %.1f, %.1f", pos.x(), pos.y(), pos.z());
    ImGui::Text("forward : %.1f, %.1f, %.1f", fwd.x(), fwd.y(), fwd.z());
    ImGui::Text("right   : %.1f, %.1f, %.1f", rht.x(), rht.y(), rht.z());
    ImGui::Text("up      : %.1f, %.1f, %.1f", up.x(), up.y(), up.z());

    ImGui::Text("rotation: %.1f, %.1f, %.1f, %.1f", rot.x(), rot.y(), rot.z(), rot.w());

    if(ImGui::CollapsingHeader("Rotation")) {
        const auto x = rot({1.0f, 0.0f, 0.0f});
        const auto y = rot({0.0f, 1.0f, 0.0f});
        const auto z = rot({0.0f, 0.0f, 1.0f});
        ImGui::Text("X axis: %.1f, %.1f, %.1f", x.x(), x.y(), x.z());
        ImGui::Text("Y axis: %.1f, %.1f, %.1f", y.x(), y.y(), y.z());
        ImGui::Text("Z axis: %.1f, %.1f, %.1f", z.x(), z.y(), z.z());

        ImGui::Separator();

        auto euler = rot.to_euler();
        ImGui::Text("pitch: %.1f°", math::to_deg(euler[math::Quaternion<>::PitchIndex]));
        ImGui::Text("yaw  : %.1f°", math::to_deg(euler[math::Quaternion<>::YawIndex]));
        ImGui::Text("roll : %.1f°", math::to_deg(euler[math::Quaternion<>::RollIndex]));
    }
}

}


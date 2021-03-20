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

#include "CameraController.h"

#include <editor/Picker.h>
#include <editor/Settings.h>
#include <editor/Selection.h>
#include <editor/EditorWorld.h>

#include <yave/camera/Camera.h>
#include <yave/utils/entities.h>

#include <external/imgui/yave_imgui.h>

#include <algorithm>

namespace editor {

template<typename T>
static bool fully_finite(const T& t) {
    return std::all_of(t.begin(), t.end(), [](float x) { return std::isfinite(x); });
}

CameraController::CameraController() {
}

void CameraController::process_generic_shortcuts(Camera& camera) {
    const ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureKeyboard || !ImGui::IsWindowHovered()) {
        return;
    }

    const CameraSettings& settings = app_settings().camera;
    math::Vec3 cam_pos = camera.position();
    math::Vec3 cam_fwd = camera.forward();
    math::Vec3 cam_rht = camera.right();

    if(ImGui::IsKeyDown(int(settings.center_on_obj))) {
        auto id = selection().selected_entity();
        if(id.is_valid()) {
            if(const auto pos = entity_position(current_world(), id)) {
                const float radius = entity_radius(current_world(), id).unwrap_or(10.0f);
                cam_pos = pos.unwrap() - cam_fwd * (radius * 1.5f);
            }
        }
    }

    const auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_rht));
    if(fully_finite(view)) {
        camera.set_view(view);
    }
}



FPSCameraController::FPSCameraController() : CameraController() {
}

void FPSCameraController::update_camera(Camera& camera, const math::Vec2ui& viewport_size) {
    const CameraSettings& settings = app_settings().camera;
    math::Vec3 cam_pos = camera.position();
    math::Vec3 cam_fwd = camera.forward();
    math::Vec3 cam_rht = camera.right();

    if(ImGui::IsWindowFocused()) {
        const float cam_speed = 500.0f;
        const float dt = cam_speed / ImGui::GetIO().Framerate;

        if(ImGui::IsKeyDown(int(settings.move_forward))) {
            cam_pos += cam_fwd * dt;
        }
        if(ImGui::IsKeyDown(int(settings.move_backward))) {
            cam_pos -= cam_fwd * dt;
        }
        if(ImGui::IsKeyDown(int(settings.move_left))) {
            cam_pos -= cam_rht * dt;
        }
        if(ImGui::IsKeyDown(int(settings.move_right))) {
            cam_pos += cam_rht * dt;
        }

        if(ImGui::IsMouseDown(1)) {
            auto delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(viewport_size);
            delta *= settings.fps_sensitivity;

            {
                const auto pitch = math::Quaternion<>::from_axis_angle(cam_rht, delta.y());
                cam_fwd = pitch(cam_fwd);
            }
            {
                const auto yaw = math::Quaternion<>::from_axis_angle(cam_fwd.cross(cam_rht), -delta.x());
                cam_fwd = yaw(cam_fwd);
                cam_rht = yaw(cam_rht);
            }

            auto euler = math::Quaternion<>::from_base(cam_fwd, cam_rht, cam_fwd.cross(cam_rht)).to_euler();
            const bool upside_down = cam_fwd.cross(cam_rht).z() < 0.0f;
            euler[math::Quaternion<>::RollIndex] = upside_down ? -math::pi<float> : 0.0f;
            const auto rotation = math::Quaternion<>::from_euler(euler);
            cam_fwd = rotation({1.0f, 0.0f, 0.0f});
            cam_rht = rotation({0.0f, 1.0f, 0.0f});
        }


        if(ImGui::IsMouseDown(2)) {
            const auto delta = ImGui::GetIO().MouseDelta;
            cam_pos -= (delta.y * cam_fwd.cross(cam_rht) + delta.x * cam_rht);
        }
    }

    const auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_rht));
    if(fully_finite(view)) {
        camera.set_view(view);
    }
}



HoudiniCameraController::HoudiniCameraController() {
}

int HoudiniCameraController::camera_key() const {
    return int(Key::Alt);
}

bool HoudiniCameraController::viewport_clicked(const PickingResult& point) {
    if(ImGui::IsKeyDown(camera_key())) {
        _picked_pos = point.world_pos;
        _picking_uvs = point.uv;
        _picking_depth = point.depth;
        _init = true;

        return true;
    }
    return false;
}

void HoudiniCameraController::update_camera(Camera& camera, const math::Vec2ui& viewport_size) {
    const bool cam_key_down = ImGui::IsKeyDown(camera_key());

    const CameraSettings& settings = app_settings().camera;
    math::Vec3 cam_pos = camera.position();
    math::Vec3 cam_fwd = camera.forward();
    math::Vec3 cam_rht = camera.right();

    for(int i = 0; i != 3; ++i) {
        if(ImGui::IsMouseClicked(i)) {
            _mouse_button = i;
        }
    }

    const bool fps = ImGui::IsMouseDown(0) && ImGui::IsMouseDown(1);
    if(fps || (_mouse_button >= 0 && !ImGui::IsMouseDown(_mouse_button))) {
        _mouse_button = -1;
    }


    if(_init) {
        bool finite = true;
        for(float f : _picked_pos) {
            finite &= std::isfinite(f);
        }

        if(!finite) {
            const math::Vec4 hp = camera.viewproj_matrix().inverse() * math::Vec4(_picking_uvs * 2.0f - 1.0f, 1.0f, 1.0f);
            _picked_pos = hp.to<3>() / hp.w();
            _picking_depth = 1.0f;
        }

        float dist = (cam_pos - _picked_pos).length();
        const math::Vec3 target_pos = cam_pos + cam_fwd * dist;
        _target_offset = target_pos - _picked_pos;
        _orig_pos = cam_pos;

        _cumulated_delta = 0.0f;

        _init = false;
    }

    math::Vec2 delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(viewport_size);
    _cumulated_delta += delta;


    if(!cam_key_down) {
        return;
    }


    if(fps) {
        // FPS
        delta *= settings.trackball_sensitivity;

        {
            const auto pitch = math::Quaternion<>::from_axis_angle(cam_rht, delta.y());
            cam_fwd = pitch(cam_fwd);
        }
        {
            const auto yaw = math::Quaternion<>::from_axis_angle(cam_fwd.cross(cam_rht), -delta.x());
            cam_fwd = yaw(cam_fwd);
            cam_rht = yaw(cam_rht);
        }

        const auto rotation = math::Quaternion<>::from_base(cam_fwd, cam_rht, cam_fwd.cross(cam_rht));
        cam_fwd = rotation({1.0f, 0.0f, 0.0f});
        cam_rht = rotation({0.0f, 1.0f, 0.0f});
    } else {
        // Trackball
        if(_mouse_button == 0) {
            math::Vec3 boom_arm = cam_pos - _picked_pos;
            delta *= settings.trackball_sensitivity;
            {
                const auto pitch = math::Quaternion<>::from_axis_angle(cam_rht, delta.y());
                boom_arm = pitch(boom_arm);
                _target_offset = pitch(_target_offset);
            }
            {
                const auto yaw = math::Quaternion<>::from_axis_angle(math::Vec3(0.0f, 0.0f, -1.0f), delta.x());
                boom_arm = yaw(boom_arm);
                _target_offset = yaw(_target_offset);

                cam_rht = yaw(cam_rht);
            }

            cam_pos = _picked_pos + boom_arm;
            cam_fwd = (_picked_pos + _target_offset - cam_pos).normalized();
        }

        // Dolly
        if(_mouse_button == 1) {
            const math::Vec3 dolly_vec = _picked_pos - _orig_pos;
            const float f = _cumulated_delta.y() * -settings.dolly_sensitivity;
            cam_pos = _picked_pos - dolly_vec * (1.0f + f);
        }

        // Pan
        if(_mouse_button == 2) {
            const math::Vec4 hp = camera.viewproj_matrix().inverse() * math::Vec4((_picking_uvs - delta) * 2.0f - 1.0f, _picking_depth, 1.0f);
            cam_pos = _orig_pos + (hp.to<3>() / hp.w()) - _picked_pos;
        }
    }


    // kill any roll that might arise from imprecisions
    {
        cam_rht.z() = 0.0f;
        cam_rht.normalize();
    }

    const auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_rht));
    if(fully_finite(view)) {
        camera.set_view(view);
    }
}

}


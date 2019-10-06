/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <editor/context/EditorContext.h>

#include <yave/camera/Camera.h>

#include <imgui/yave_imgui.h>

namespace editor {

CameraController::CameraController(ContextPtr ctx) : ContextLinked(ctx) {
}



FPSCameraController::FPSCameraController(ContextPtr ctx) : CameraController(ctx) {
}

void FPSCameraController::update_camera(Camera& camera, const math::Vec2ui& viewport_size) {
	math::Vec3 cam_pos = camera.position();
	math::Vec3 cam_fwd = camera.forward();
	math::Vec3 cam_lft = camera.left();

	if(ImGui::IsWindowFocused()) {
		float cam_speed = 500.0f;
		float dt = cam_speed / ImGui::GetIO().Framerate;

		if(ImGui::IsKeyDown(int(context()->settings().camera().move_forward))) {
			cam_pos += cam_fwd * dt;
		}
		if(ImGui::IsKeyDown(int(context()->settings().camera().move_backward))) {
			cam_pos -= cam_fwd * dt;
		}
		if(ImGui::IsKeyDown(int(context()->settings().camera().move_left))) {
			cam_pos += cam_lft * dt;
		}
		if(ImGui::IsKeyDown(int(context()->settings().camera().move_right))) {
			cam_pos -= cam_lft * dt;
		}


		if(ImGui::IsMouseDown(1)) {
			auto delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(ImGui::GetWindowSize());
			delta *= context()->settings().camera().sensitivity;

			{
				auto pitch = math::Quaternion<>::from_axis_angle(cam_lft, delta.y());
				cam_fwd = pitch(cam_fwd);
			}
			{
				auto yaw = math::Quaternion<>::from_axis_angle(cam_fwd.cross(cam_lft), -delta.x());
				cam_fwd = yaw(cam_fwd);
				cam_lft = yaw(cam_lft);
			}

			auto euler = math::Quaternion<>::from_base(cam_fwd, cam_lft, cam_fwd.cross(cam_lft)).to_euler();
			bool upside_down = cam_fwd.cross(cam_lft).z() < 0.0f;
			euler[math::Quaternion<>::RollIndex] = upside_down ? -math::pi<float> : 0.0f;
			auto rotation = math::Quaternion<>::from_euler(euler);
			cam_fwd = rotation({1.0f, 0.0f, 0.0f});
			cam_lft = rotation({0.0f, 1.0f, 0.0f});
		}


		if(ImGui::IsMouseDown(2)) {
			auto delta = ImGui::GetIO().MouseDelta;
			cam_pos -= (delta.y * cam_fwd.cross(cam_lft) + delta.x * cam_lft);
		}
	}

	float fov = math::to_rad(60.0f);
	auto proj = math::perspective(fov, float(viewport_size.x()) / float(viewport_size.y()), 1.0f);
	auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_lft));
	camera.set_proj(proj);
	camera.set_view(view);
}

}

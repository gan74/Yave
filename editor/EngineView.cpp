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

#include "EngineView.h"

#include <editor/context/EditorContext.h>
#include <yave/renderer/renderer.h>

#include <imgui/yave_imgui.h>

namespace editor {

EngineView::EngineView(ContextPtr cptr) :
		Widget(ICON_FA_DESKTOP " Engine View"),
		ContextLinked(cptr),
		_ibl_data(std::make_shared<IBLData>(device())),
		_scene_view(&context()->world()),
		_gizmo(context(), &_scene_view) {
	;
}

EngineView::~EngineView() {
	context()->remove_scene_view(&_scene_view);
}

void EngineView::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();
	update();

	TextureView* output = nullptr;

	{
		FrameGraph graph(context()->resource_pool());
		DefaultRenderer renderer = DefaultRenderer::create(graph, _scene_view, content_size(), _ibl_data);

		FrameGraphImageId output_image = renderer.tone_mapping.tone_mapped;
		{
			FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");
			builder.add_texture_input(output_image, PipelineStage::FragmentBit);
			builder.set_render_func([&output, output_image](CmdBufferRecorder& rec, const FrameGraphPass* pass) {
					auto out = std::make_unique<TextureView>(pass->resources()->image<ImageUsage::TextureBit>(output_image));
					output = out.get();
					rec.keep_alive(std::move(out));
				});
		}

		std::move(graph).render(recorder);
	}

	// ImGui
	{
		if(output) {
			ImGui::GetWindowDrawList()->AddImage(output,
				position() + math::Vec2(ImGui::GetWindowContentRegionMin()),
				position() + math::Vec2(ImGui::GetWindowContentRegionMax()));

		}

		/*if(&context()->scene().scene_view() == &_scene_view)*/ {
			_gizmo.paint(recorder, token);
			if(!_gizmo.is_dragging()) {
				update_selection();
			}
		}
	}
}

void EngineView::update() {
	if(ImGui::IsWindowHovered()) {
		if(ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2)) {
			ImGui::SetWindowFocus();
			update_picking();
		}
	}

	if(ImGui::IsWindowFocused()) {
		context()->set_scene_view(&_scene_view);
	}

	// process inputs
	update_camera();
}

void EngineView::update_picking() {
	math::Vec2 viewport = ImGui::GetWindowSize();
	math::Vec2 offset = ImGui::GetWindowPos();

	math::Vec2 uv = ((math::Vec2(ImGui::GetIO().MousePos) - offset) / viewport);
	_picked_pos = context()->picking_manager().pick_sync(uv).world_pos;
}

void EngineView::update_camera() {
	auto size = content_size();
	auto& camera = _scene_view.camera();

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
	auto proj = math::perspective(fov, float(size.x()) / float(size.y()), 1.0f);
	auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_lft));
	camera.set_proj(proj);
	camera.set_view(view);

	/*auto& camera = _scene_view.camera();

	math::Vec3 cam_pos = camera.position();
	math::Vec3 cam_fwd = camera.forward();
	math::Vec3 cam_lft = camera.left();
	math::Vec3 cam_up = camera.up();

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
	}

	if(ImGui::IsMouseDown(1)) {
		math::Vec3 view_vec = cam_pos - _picked_pos;

		// trackball
		auto delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(ImGui::GetWindowSize());
		delta *= context()->settings().camera().sensitivity;
		{
			auto pitch = math::Quaternion<>::from_axis_angle(cam_lft, delta.y());
			view_vec = pitch(view_vec);
		}
		{
			auto yaw = math::Quaternion<>::from_axis_angle(cam_up, delta.x());
			view_vec = yaw(view_vec);
		}
		cam_pos = view_vec + _picked_pos;
	}

	auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_lft));
	camera.set_view(view);*/
}

void EngineView::update_selection() {
	if(!ImGui::IsWindowHovered() || !ImGui::IsMouseClicked(0)) {
		return;
	}

	/*StaticMeshInstance* selected = nullptr;
	float score = std::numeric_limits<float>::max();
	for(const auto& tr : context()->scene().scene().static_meshes()) {
		auto [pos, rot, scale] = tr->transform().decompose();
		unused(rot);

		float radius = tr->radius() * std::max({scale.x(), scale.y(), scale.z()});
		float sc = (pos - _picked_pos).length() / radius;

		if(sc < score) {
			selected = tr.get();
			score = sc;
		}
	}
	context()->selection().set_selected(selected);*/
}

}

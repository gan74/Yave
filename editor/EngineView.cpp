/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <yave/scene/SceneView.h>
#include <yave/renderers/ToneMapper.h>
#include <yave/window/EventHandler.h>

#include <y/math/Volume.h>
#include <y/io/File.h>

#include <imgui/imgui.h>

namespace editor {

EngineView::EngineView(ContextPtr cptr) :
		ContextLinked(cptr),
		_ibl_data(new IBLData(device())),
		_gizmo(context()) {
}

math::Vec2ui EngineView::render_size() const {
	return _renderer ? _renderer->output().size() : math::Vec2ui();
}

void EngineView::create_renderer(const math::Vec2ui& size) {
	auto scene		= Node::Ptr<SceneRenderer>(new SceneRenderer(device(), *context()->scene_view()));
	auto gbuffer	= Node::Ptr<GBufferRenderer>(new GBufferRenderer(scene, size));
	auto deferred	= Node::Ptr<TiledDeferredRenderer>(new TiledDeferredRenderer(gbuffer, _ibl_data));
	auto tonemap	= Node::Ptr<SecondaryRenderer>(new ToneMapper(deferred));

	_renderer		= Node::Ptr<FramebufferRenderer>(new FramebufferRenderer(tonemap, size));
	_view			= std::make_shared<TextureView>(_renderer->output());
}

void EngineView::paint(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	math::Vec2 viewport = ImGui::GetWindowSize();

	if(context()->is_scene_empty()) {
		ImGui::Text("Empty scene");
		return;
	}

	if(!_renderer || viewport != render_size()) {
		create_renderer(viewport);
		return;
	}

	if(_renderer) {
		// process inputs
		update_camera();

		// render engine
		{
			RenderingPipeline pipeline(_renderer);
			pipeline.render(recorder, token);
#warning barrier
			// so we don't have to wait when resizing
			recorder.keep_alive(std::make_pair(_renderer, _view));
		}


		math::Vec2 offset = ImGui::GetWindowPos();
		ImGui::GetWindowDrawList()->AddImage(_view.get(), offset, offset + viewport);

		for(const auto& light : context()->scene()->lights()) {
			float s = 18.0f;
			auto screen = context()->to_window_pos(light->position());
			ImGui::GetWindowDrawList()->AddImageQuad(&context()->icons()->light,
					screen + math::Vec2(s, -s),
					screen + math::Vec2(-s, -s),
					screen + math::Vec2(-s, s),
					screen + math::Vec2(s, s)
				);
		}

		_gizmo.paint(recorder, token);
		if(!_gizmo.is_dragging()) {
			update_selection();
		}
	}


}

void EngineView::update_selection() {
	if(!ImGui::IsWindowHovered() || !ImGui::IsMouseClicked(0)) {
		return;
	}

	math::Vec2 viewport = ImGui::GetWindowSize();
	math::Vec2 offset = ImGui::GetWindowPos();

	auto inv_matrix = context()->scene_view()->camera().inverse_matrix();
	auto cam_pos = context()->scene_view()->camera().position();

	math::Vec2 ndc = ((math::Vec2(ImGui::GetIO().MousePos) - offset) / viewport) * 2.0f - 1.0f;
	math::Vec4 h_world = inv_matrix * math::Vec4(ndc, 0.5f, 1.0f);
	math::Vec3 world = h_world.to<3>() / h_world.w();

	math::Ray<> ray(cam_pos, world - cam_pos);

	float distance = std::numeric_limits<float>::max();
	for(const auto& tr : context()->scene()->static_meshes()) {
		auto [pos, rot, sc] = tr->transform().decompose();
		unused(rot);

		float scale = std::max({sc.x(), sc.y(), sc.z()});
		float dist = (pos - cam_pos).length();

		if(ray.intersects(pos, tr->radius() * scale) && dist < distance) {
			context()->set_selected(tr.get());
			distance = dist;
		}
	}
	if(distance == std::numeric_limits<float>::max()) {
		context()->set_selected(nullptr);
	}
}

void EngineView::update_camera() {
	// TODO check keyboard focus

	auto size = render_size();
	auto& camera = context()->scene_view()->camera();
	math::Vec3 cam_pos = camera.position();
	math::Vec3 cam_fwd = camera.forward();
	math::Vec3 cam_lft = camera.left();


	float cam_speed = 500.0f;
	float dt = cam_speed / ImGui::GetIO().Framerate;

	if(ImGui::IsKeyDown(int(context()->camera_settings.move_forward))) {
		cam_pos += cam_fwd * dt;
	}
	if(ImGui::IsKeyDown(int(context()->camera_settings.move_backward))) {
		cam_pos -= cam_fwd * dt;
	}
	if(ImGui::IsKeyDown(int(context()->camera_settings.move_left))) {
		cam_pos += cam_lft * dt;
	}
	if(ImGui::IsKeyDown(int(context()->camera_settings.move_right))) {
		cam_pos -= cam_lft * dt;
	}

	float fov = math::to_rad(60.0f);

	if(ImGui::IsMouseDown(1)) {
		auto delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(ImGui::GetWindowSize());
		delta *= context()->camera_settings.camera_sensitivity;

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


	auto proj = math::perspective(fov, float(size.x()) / float(size.y()), 1.0f);
	auto view = math::look_at(cam_pos, cam_pos + cam_fwd, cam_fwd.cross(cam_lft));
	camera.set_proj(proj);
	camera.set_view(view);
}

}

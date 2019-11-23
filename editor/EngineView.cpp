/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <editor/widgets/AssetSelector.h>
#include <editor/context/EditorContext.h>

#include <yave/renderer/renderer.h>

#include <imgui/yave_imgui.h>

namespace editor {

EngineView::EngineView(ContextPtr cptr) :
		Widget(ICON_FA_DESKTOP " Engine View", ImGuiWindowFlags_MenuBar),
		ContextLinked(cptr),
		_ibl_probe(context()->device()->device_resources().empty_probe()),
		_scene_view(&context()->world()),
		_camera_controller(std::make_unique<HoudiniCameraController>(context())),
		_gizmo(context(), &_scene_view) {
}

EngineView::~EngineView() {
	context()->remove_scene_view(&_scene_view);
}

void EngineView::before_paint() {
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, math::Vec4(0.0f));
	ImGui::PushStyleColor(ImGuiCol_Border, math::Vec4(0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, math::Vec2(2.0f, 0.0f));
}

void EngineView::after_paint() {
	ImGui::PopStyleColor(2);
	// poped during menu rendering
	// ImGui::PopStyleVar(1);
}

void EngineView::draw(CmdBufferRecorder& recorder) {
	TextureView* output = nullptr;
	FrameGraph graph(context()->resource_pool());

	math::Vec2ui output_size = content_size();
	const EditorRenderer renderer = EditorRenderer::create(context(), graph, _scene_view, output_size, _ibl_probe, _settings);


	{
		FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");

		const auto output_image = builder.declare_image(vk::Format::eR8G8B8A8Unorm, output_size);
		const auto buffer = builder.declare_typed_buffer<u32>(1);

		const auto gbuffer = renderer.renderer.gbuffer;
		builder.add_image_input_usage(output_image, ImageUsage::TextureBit);
		builder.add_color_output(output_image);
		builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
		builder.add_uniform_input(gbuffer.color, 0, PipelineStage::FragmentBit);
		builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::FragmentBit);
		builder.add_uniform_input(renderer.color, 0, PipelineStage::FragmentBit);
		builder.add_uniform_input(buffer);
		builder.map_update(buffer);
		builder.set_render_func([=, index = u32(_view), &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
				auto out = std::make_unique<TextureView>(self->resources().image<ImageUsage::TextureBit>(output_image));
				output = out.get();
				recorder.keep_alive(std::move(out));

				TypedMapping<u32> mapping = self->resources().mapped_buffer(buffer);
				mapping[0] = index;

				auto render_pass = recorder.bind_framebuffer(self->framebuffer());
				const auto* material = context()->resources()[EditorResources::CopyTargetMaterialTemplate];
				render_pass.bind_material(material, {self->descriptor_sets()[0]});
				render_pass.draw(vk::DrawIndirectCommand(6, 1));
			});
	}

	std::move(graph).render(recorder);

	if(output) {
		//u32 color = ImGui::IsWindowFocused() ? 0xFFFFFFFF : 0xFF0000FF;
		ImGui::GetWindowDrawList()->AddImage(output,
			position() + math::Vec2(ImGui::GetWindowContentRegionMin()),
			position() + math::Vec2(ImGui::GetWindowContentRegionMax())/*,
			math::Vec2(0.0f), math::Vec2(1.0f), color*/);
	}
}

void EngineView::paint_ui(CmdBufferRecorder& recorder, const FrameToken&) {
	y_profile();

	update_proj();

	draw_menu_bar();
	draw(recorder);
	_gizmo.draw();

	update();
}

bool EngineView::is_clicked() const {
	return ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2);
}


void EngineView::update_proj() {
	const CameraSettings& settings = context()->settings().camera();
	math::Vec2ui viewport_size = content_size();

	const float fov = math::to_rad(settings.fov);
	const auto proj = math::perspective(fov, float(viewport_size.x()) / float(viewport_size.y()), settings.z_near);
	_scene_view.camera().set_proj(proj);
}

void EngineView::update() {
	_gizmo.set_allow_drag(true);

	const math::Vec2 mouse_pos = math::Vec2(ImGui::GetIO().MousePos) - math::Vec2(ImGui::GetWindowPos());
	const auto less = [](const math::Vec2& a, const math::Vec2& b) { return a.x() < b.x() && a.y() < b.y(); };

	bool focussed = ImGui::IsWindowFocused();
	const bool hovered =
			ImGui::IsWindowHovered() &&
			less(mouse_pos, ImGui::GetWindowContentRegionMax()) &&
			less(ImGui::GetWindowContentRegionMin(), mouse_pos);

	if(hovered && is_clicked()) {
		ImGui::SetWindowFocus();
		focussed = true;

		update_picking();
	}

	if(focussed) {
		context()->set_scene_view(&_scene_view);
	}


	if(hovered && !_gizmo.is_dragging() && _camera_controller) {
		auto& camera = _scene_view.camera();
		_camera_controller->process_generic_shortcuts(camera);
		if(focussed) {
			auto size = content_size();
			_camera_controller->update_camera(camera, size);
		}
	}
}

void EngineView::update_picking() {
	math::Vec2ui viewport_size = content_size();
	math::Vec2 offset = ImGui::GetWindowPos();
	const math::Vec2 mouse = ImGui::GetIO().MousePos;
	const math::Vec2 uv = (mouse - offset - math::Vec2(ImGui::GetWindowContentRegionMin())) / math::Vec2(viewport_size);

	if(uv.x() < 0.0f || uv.y() < 0.0f ||
	   uv.x() > 1.0f || uv.y() > 1.0f) {

		return;
	}

	const auto picking_data = context()->picking_manager().pick_sync(_scene_view, uv, viewport_size);
	if(_camera_controller && _camera_controller->viewport_clicked(picking_data)) {
		// event has been eaten by the camera controller, don't proceed further
		_gizmo.set_allow_drag(false);
		return;
	}

	if(ImGui::IsMouseClicked(0)) {
		if(!_gizmo.is_dragging()) {
			ecs::EntityId picked_id = picking_data.hit() ? context()->world().id_from_index(picking_data.entity_index) : ecs::EntityId();
			context()->selection().set_selected(picked_id);
		}
	}
}

void EngineView::draw_menu_bar() {
	if(ImGui::BeginMenuBar()) {
		ImGui::PopStyleVar(1);

		if(ImGui::BeginMenu("Render")) {
			ImGui::MenuItem("Editor entities", nullptr, &_settings.enable_editor_entities);

			ImGui::Separator();
			if(ImGui::BeginMenu("Tone mapping")) {
				ToneMappingSettings& settings = _settings.renderer_settings.tone_mapping;
				ImGui::MenuItem("Auto exposure", nullptr, &settings.auto_exposure);
				ImGui::SliderFloat("Key", &settings.key_value, 0.01f, 1.0f);
				ImGui::EndMenu();
			}

			ImGui::Separator();
			{
				const char* output_names[] = {
						"Lit", "Albedo", "Normals", "Metallic", "Roughness", "Depth"
					};
				for(usize i = 0; i != usize(RenderView::MaxRenderViews); ++i) {
					bool selected = usize(_view) == i;
					ImGui::MenuItem(output_names[i], nullptr, &selected);
					if(selected) {
						_view = RenderView(i);
					}
				}
			}

			ImGui::Separator();
			if(ImGui::BeginMenu("IBL")) {
				if(ImGui::MenuItem("Set IBL probe")) {
					add_child<AssetSelector>(context(), AssetType::Image)->set_selected_callback(
						[this](AssetId id) {
							if(auto tex = context()->loader().load<Texture>(id)) {
								_ibl_probe = std::make_shared<IBLProbe>(IBLProbe::from_equirec(*tex.unwrap()));
							}
							return true;
						});
				}
				if(ImGui::MenuItem("Reset IBL probe")) {
					_ibl_probe = context()->device()->device_resources().empty_probe();
				}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Camera")) {
			if(ImGui::MenuItem("Reset camera")) {
				_scene_view.camera().set_view(Camera().view_matrix());
			}
			ImGui::EndMenu();
		}

		draw_gizmo_tool_bar();

		ImGui::EndMenuBar();
	}
}

void EngineView::draw_gizmo_tool_bar() {
	auto gizmo_mode = _gizmo.mode();
	auto gizmo_space = _gizmo.space();
	{
		if(ImGui::MenuItem(ICON_FA_ARROWS_ALT, nullptr, false, gizmo_mode != Gizmo::Translate)) {
			gizmo_mode = Gizmo::Translate;
		}
		if(ImGui::MenuItem(ICON_FA_SYNC_ALT, nullptr, false, gizmo_mode != Gizmo::Rotate)) {
			gizmo_mode = Gizmo::Rotate;
		}
	}
	{
		if(ImGui::MenuItem(ICON_FA_MOUNTAIN, nullptr, false, gizmo_space != Gizmo::World)) {
			gizmo_space = Gizmo::World;
		}
		if(ImGui::MenuItem(ICON_FA_CUBE, nullptr, false, gizmo_space != Gizmo::Local)) {
			gizmo_space = Gizmo::Local;
		}
	}

	if(is_focussed()) {
		const UiSettings& settings = context()->settings().ui();
		if(ImGui::IsKeyReleased(int(settings.change_gizmo_mode))) {
			gizmo_mode = Gizmo::Mode(!usize(gizmo_mode));
		}
		if(ImGui::IsKeyReleased(int(settings.change_gizmo_space))) {
			gizmo_space = Gizmo::Space(!usize(gizmo_space));
		}
	}
	_gizmo.set_mode(gizmo_mode);
	_gizmo.set_space(gizmo_space);
}

}

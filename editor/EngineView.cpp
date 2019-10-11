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
#include <yave/components/TransformableComponent.h>

#include <yave/renderer/renderer.h>

#include <imgui/yave_imgui.h>

namespace editor {

EngineView::EngineView(ContextPtr cptr) :
		Widget(ICON_FA_DESKTOP " Engine View"),
		ContextLinked(cptr),
		_ibl_data(std::make_shared<IBLData>(device())),
		_scene_view(&context()->world()),
		_camera_controller(std::make_unique<HoudiniCameraController>(context())),
		_gizmo(context(), &_scene_view) {
}

EngineView::~EngineView() {
	context()->remove_scene_view(&_scene_view);
}

void EngineView::draw(CmdBufferRecorder& recorder) {
	TextureView* output = nullptr;
	FrameGraph graph(context()->resource_pool());

	EditorRenderer renderer = EditorRenderer::create(context(), graph, _scene_view, content_size(), _ibl_data, _settings);

	FrameGraphImageId output_image = renderer.out;
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

	if(output) {
		//u32 color = ImGui::IsWindowFocused() ? 0xFFFFFFFF : 0xFF0000FF;
		ImGui::GetWindowDrawList()->AddImage(output,
			position() + math::Vec2(ImGui::GetWindowContentRegionMin()),
			position() + math::Vec2(ImGui::GetWindowContentRegionMax())/*,
			math::Vec2(0.0f), math::Vec2(1.0f), color*/);
	}
}

void EngineView::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();

	draw(recorder);
	_gizmo.paint(recorder, token);
	draw_rendering_menu();

	// this needs to be after the gizmo is painted to avoid drag and selection interferences
	update();
}

void EngineView::draw_rendering_menu() {
	ImGui::SetCursorPos(math::Vec2(ImGui::GetWindowContentRegionMin()) + math::Vec2(8));

	if(ImGui::Button(ICON_FA_COG)) {
		ImGui::OpenPopup("###renderersettings");
	}

	if(ImGui::BeginPopup("###renderersettings")) {
		ImGui::MenuItem("Editor entities", nullptr, &_settings.enable_editor_entities);

		if(ImGui::BeginMenu("Tone mapping")) {
			ToneMappingSettings& settings = _settings.renderer_settings.tone_mapping;
			ImGui::MenuItem("Auto exposure", nullptr, &settings.auto_exposure);
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}

}

bool EngineView::is_clicked() const {
	return ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2);
}

void EngineView::update() {
	bool hovered = ImGui::IsWindowHovered();
	bool focussed = ImGui::IsWindowFocused();

	if(hovered && is_clicked()) {
		ImGui::SetWindowFocus();
		focussed = true;

		pick();
	}

	if(focussed) {
		context()->set_scene_view(&_scene_view);
	}

	if(focussed && hovered && _camera_controller) {
		auto size = content_size();
		auto& camera = _scene_view.camera();
		_camera_controller->update_camera(camera, size);
	}
}

void EngineView::pick() {
	math::Vec2ui viewport_size = content_size();
	math::Vec2 offset = ImGui::GetWindowPos();
	math::Vec2 mouse = ImGui::GetIO().MousePos;
	math::Vec2 uv = (mouse - offset - math::Vec2(ImGui::GetWindowContentRegionMin())) / math::Vec2(viewport_size);

	if(uv.x() < 0.0f || uv.y() < 0.0f) {
		return;
	}

	auto picking_data = context()->picking_manager().pick_sync(_scene_view, uv, viewport_size);
	if(_camera_controller && _camera_controller->viewport_clicked(picking_data)) {
		// event has been eaten by the camera controller, don't proceed further
		return;
	}

	if(ImGui::IsMouseClicked(0)) {
		if(!_gizmo.is_dragging()) {
			ecs::EntityId picked_id = picking_data.hit() ? context()->world().id_from_index(picking_data.entity_index) : ecs::EntityId();
			context()->selection().set_selected(picked_id);
		}
	}
}

}

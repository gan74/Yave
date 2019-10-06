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

void EngineView::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();
	update();

	TextureView* output = nullptr;

	{
		FrameGraph graph(context()->resource_pool());
		EditorRenderer renderer = EditorRenderer::create(context(), graph, _scene_view, content_size(), _ibl_data);

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
	math::Vec2ui viewport_size = content_size();
	math::Vec2 offset = ImGui::GetWindowPos();
	math::Vec2 mouse = ImGui::GetIO().MousePos;
	math::Vec2 uv = (mouse - offset - math::Vec2(ImGui::GetWindowContentRegionMin())) / math::Vec2(viewport_size);

	if(uv.x() < 0.0f || uv.y() < 0.0f) {
		return;
	}

	auto picking_data = context()->picking_manager().pick_sync(uv, viewport_size);
	if(_camera_controller && _camera_controller->viewport_clicked(picking_data)) {
		// event has been eaten by the camera controller, don't proceed further
		return;
	}

	_picked_entity_id = picking_data.hit() ? context()->world().id_from_index(picking_data.entity_index) : ecs::EntityId();
}

void EngineView::update_selection() {
	y_profile();

	if(!ImGui::IsWindowHovered() || !ImGui::IsMouseClicked(0)) {
		return;
	}

	context()->selection().set_selected(_picked_entity_id);
}

void EngineView::update_camera() {
	if(_camera_controller) {
		auto size = content_size();
		auto& camera = _scene_view.camera();

		_camera_controller->update_camera(camera, size);
	}
}

}

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

#include "MaterialPreview.h"
#include "AssetSelector.h"

#include <editor/context/EditorContext.h>

#include <yave/renderer/renderer.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/entities/entities.h>
#include <yave/utils/color.h>

#include <editor/utils/ui.h>

#include <imgui/yave_imgui.h>

namespace editor {


MaterialPreview::MaterialPreview(ContextPtr cptr) :
		Widget(ICON_FA_BRUSH " Material Preview"),
		ContextLinked(cptr),
		_ibl_probe(device()->device_resources().ibl_probe()),
		_resource_pool(std::make_shared<FrameGraphResourcePool>(device())) {

	set_object(PreviewObject::Sphere);
}

void MaterialPreview::refresh() {
	//_world.flush_reload(context()->loader());
}

void MaterialPreview::set_material(const AssetPtr<Material>& material) {
	_material = material;
	reset_world();
}

void MaterialPreview::set_object(const AssetPtr<StaticMesh>& mesh) {
	_mesh = mesh;
	reset_world();
}

void MaterialPreview::set_object(PreviewObject obj) {
	switch(obj) {
		case PreviewObject::Cube:
			_mesh = device()->device_resources()[DeviceResources::CubeMesh];
		break;

		default:
			_mesh = device()->device_resources()[DeviceResources::SphereMesh];
	}
	reset_world();
}


void MaterialPreview::update_camera() {
	if(ImGui::IsMouseDown(0) && is_mouse_inside()) {
		math::Vec2 delta = math::Vec2(ImGui::GetIO().MouseDelta) / math::Vec2(content_size());
		delta *= context()->settings().camera().trackball_sensitivity;

		const float pi_2 = (math::pi<float> * 0.5f) - 0.001f;
		_angle.y() = std::clamp(_angle.y() + delta.y(), -pi_2, pi_2);
		_angle.x() += delta.x();
	}

	{
		const float cos_y = std::cos(_angle.y());
		const math::Vec3 cam = math::Vec3(std::sin(_angle.x()) * cos_y, std::cos(_angle.x()) * cos_y, std::sin(_angle.y()));

		_view.camera().set_view(math::look_at(cam * _cam_distance, math::Vec3(), math::Vec3(0.0f, 0.0f, 1.0f)));
	}
}


void MaterialPreview::reset_world() {
	_world = std::make_unique<ecs::EntityWorld>();
	_view = SceneView(_world.get());

	if(!_mesh.is_empty() && !_material.is_empty()) {
		const ecs::EntityId id = _world->create_entity(StaticMeshArchetype());
		*_world->component<StaticMeshComponent>(id) = StaticMeshComponent(_mesh, _material);

		const float radius = _mesh->radius();
		_cam_distance = std::sqrt(3 * radius * radius) * 1.5f;
	}
}

void MaterialPreview::paint_mesh_menu() {
	if(ImGui::BeginPopup("##contextmenu")) {
		if(ImGui::MenuItem("Sphere")) {
			set_object(PreviewObject::Sphere);
		}
		if(ImGui::MenuItem("Cube")) {
			set_object(PreviewObject::Cube);
		}
		ImGui::Separator();
		if(ImGui::MenuItem("Custom")) {
			add_child<AssetSelector>(context(), AssetType::Mesh)->set_selected_callback(
				[this](AssetId id) {
					if(auto mesh = context()->loader().load_res<StaticMesh>(id)) {
						set_object(mesh.unwrap());
					}
					return true;
				});
		}
		ImGui::EndPopup();
	}
}

void MaterialPreview::paint_ui(CmdBufferRecorder& recorder, const FrameToken&) {
	y_profile();

	update_camera();

	TextureView* output = nullptr;

	{
		FrameGraph graph(_resource_pool);
		const DefaultRenderer renderer = DefaultRenderer::create(graph, _view, content_size(), _ibl_probe);

		FrameGraphPassBuilder builder = graph.add_pass("ImGui texture pass");

		const auto output_image = builder.declare_copy(renderer.lighting.lit);
		builder.set_render_func([=, &output](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
				auto out = std::make_unique<TextureView>(self->resources().image<ImageUsage::TextureBit>(output_image));
				output = out.get();
				recorder.keep_alive(std::move(out));
			});

		const auto region = recorder.region("Material preview render", math::Vec4(0.7f, 0.7f, 0.7f, 1.0f));
		std::move(graph).render(recorder);
	}

	if(output) {
		const math::Vec2 top_left = ImGui::GetCursorPos();
		const float width = ImGui::GetContentRegionAvail().x;
		ImGui::Image(output, ImVec2(width, width));
		const math::Vec2 bottom = ImGui::GetCursorPos();

		ImGui::SetCursorPos(top_left + math::Vec2(4.0f));
		if(ImGui::Button(ICON_FA_CIRCLE)) {
			add_child<AssetSelector>(context(), AssetType::Image)->set_selected_callback(
				[this](AssetId id) {
					if(auto tex = context()->loader().load_res<Texture>(id)) {
						_ibl_probe = std::make_shared<IBLProbe>(IBLProbe::from_equirec(*tex.unwrap()));
					}
					return true;
				});
		}

		ImGui::SameLine();

		if(ImGui::Button(ICON_FA_CUBE)) {
			ImGui::OpenPopup("##contextmenu");
		}

		paint_mesh_menu();

		ImGui::SetCursorPos(bottom);
	}
}

}

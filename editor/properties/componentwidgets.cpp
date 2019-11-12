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

#include "componentwidgets.h"

#include <editor/context/EditorContext.h>

#include <editor/components/EditorComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

#include <yave/utils/color.h>
#include <editor/utils/ui.h>
#include <editor/widgets/AssetSelector.h>

#include <imgui/yave_imgui.h>

#include <mutex>

namespace editor {
namespace detail {

static ComponentWidgetData* first_component = nullptr;

void register_component_widget(ComponentWidgetData* data) {
	data->next = first_component;
	first_component = data;
}

}

void draw_component_widgets(ContextPtr ctx, ecs::EntityId id) {
	for(detail::ComponentWidgetData* data = detail::first_component; data; data = data->next) {
		y_debug_assert(data->func);
		data->func(ctx, id);
	}
}





/**************************************************************************
*								Lights
**************************************************************************/

template<typename T>
static void light_widget(T* light) {
	int color_flags = ImGuiColorEditFlags_NoSidePreview |
					  // ImGuiColorEditFlags_NoSmallPreview |
					  ImGuiColorEditFlags_NoAlpha |
					  ImGuiColorEditFlags_Float |
					  ImGuiColorEditFlags_InputRGB;

	math::Vec4 color(light->color(), 1.0f);
	if(ImGui::ColorButton("Color", color, color_flags)) {
		ImGui::OpenPopup("Color");
	}
	ImGui::SameLine();
	ImGui::Text("Light color");

	if(ImGui::BeginPopup("Color")) {
		ImGui::ColorPicker3("##lightpicker", light->color().begin(), color_flags);

		float kelvin = std::clamp(rgb_to_k(light->color()), 1000.0f, 12000.0f);
		if(ImGui::SliderFloat("", &kelvin, 1000.0f, 12000.0f, "%.0f°K")) {
			light->color() = k_to_rbg(kelvin);
		}

		ImGui::EndPopup();
	}

	ImGui::DragFloat("Intensity", &light->intensity(), 1.0f, 0.0f, std::numeric_limits<float>::max());
}

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	PointLightComponent* light = ctx->world().component<PointLightComponent>(id);
	if(!light) {
		return;
	}

	if(!ImGui::CollapsingHeader("Point light", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}


	light_widget(light);

	ImGui::BeginGroup();
	ImGui::DragFloat("Radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
	ImGui::DragFloat("Falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f", 2.0f);
	ImGui::EndGroup();
}

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	DirectionalLightComponent* light = ctx->world().component<DirectionalLightComponent>(id);
	if(!light) {
		return;
	}

	if(!ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	light_widget(light);

	ImGui::BeginGroup();
	ImGui::InputFloat3("Direction", light->direction().data(), "%.2f");
	ImGui::EndGroup();
}



/**************************************************************************
*								Meshes
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	if(!ctx->world().component<StaticMeshComponent>(id)) {
		return;
	}
	if(!ImGui::CollapsingHeader("Static mesh")) {
		return;
	}

	auto static_mesh = [ctx, id]() -> StaticMeshComponent* { return ctx->world().component<StaticMeshComponent>(id); };
	{
		// material
		if(imgui::asset_selector(ctx, static_mesh()->material().id(), AssetType::Material, "Material")) {
			ctx->ui().add<AssetSelector>(AssetType::Material)->set_selected_callback(
				[=](AssetId asset) {
					if(auto material = ctx->loader().load<Material>(asset)) {
						if(StaticMeshComponent* comp = static_mesh()) {
							comp->material() = material.unwrap();
						}
					}
					return true;
				});
		}

		// mesh
		if(imgui::asset_selector(ctx, static_mesh()->mesh().id(), AssetType::Mesh, "Mesh")) {
			ctx->ui().add<AssetSelector>(AssetType::Mesh)->set_selected_callback(
				[=](AssetId asset) {
					if(auto mesh = ctx->loader().load<StaticMesh>(asset)) {
						if(StaticMeshComponent* comp = static_mesh()) {
							comp->mesh() = mesh.unwrap();
						}
					}
					return true;
				});
		}
	}
}



/**************************************************************************
*								Editor
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	EditorComponent* component = ctx->world().component<EditorComponent>(id);
	if(!component) {
		return;
	}

	if(!ImGui::CollapsingHeader("Entity")) {
		return;
	}

	const core::String& name = component->name();

	std::array<char, 1024> name_buffer;
	std::fill(name_buffer.begin(), name_buffer.end(), 0);
	std::copy(name.begin(), name.end(), name_buffer.begin());

	if(ImGui::InputText("Name", name_buffer.begin(), name_buffer.size())) {
		component->set_name(name_buffer.begin());
	}
}

}

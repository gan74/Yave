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
#include <yave/components/SkyComponent.h>
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
	usize index = 0;
	for(detail::ComponentWidgetData* data = detail::first_component; data; data = data->next) {
		y_debug_assert(data->func);

		ImGui::PushID(fmt("wid %", index++).data());
		data->func(ctx, id);
		ImGui::PopID();
	}
}





/**************************************************************************
*								Lights
**************************************************************************/

template<typename T>
static void light_widget(T* light) {
	const int color_flags = ImGuiColorEditFlags_NoSidePreview |
					  // ImGuiColorEditFlags_NoSmallPreview |
					  ImGuiColorEditFlags_NoAlpha |
					  ImGuiColorEditFlags_Float |
					  ImGuiColorEditFlags_InputRGB;

	const math::Vec4 color(light->color(), 1.0f);
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

	ImGui::DragFloat("Intensity", &light->intensity(), 0.1f, 0.0f, std::numeric_limits<float>::max());
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

	ImGui::DragFloat("Radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
	ImGui::DragFloat("Falloff", &light->falloff(), 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.2f", 2.0f);
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

	ImGui::InputFloat3("Direction", light->direction().data(), "%.2f");
}


editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	SkyComponent* sky = ctx->world().component<SkyComponent>(id);
	if(!sky) {
		return;
	}

	if(!ImGui::CollapsingHeader("Sky", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	light_widget(&sky->sun());

	ImGui::InputFloat3("Direction", sky->sun().direction().data(), "%.2f");

	math::Vec3 beta = sky->beta_rayleight() * 1e6f;
	if(ImGui::InputFloat3("Beta", beta.data(), "%.2f")) {
		sky->beta_rayleight() = beta * 1e-6f;
	}

	ImGui::SameLine();

	if(ImGui::Button(ICON_FA_GLOBE_AFRICA)) {
		sky->beta_rayleight() = SkyComponent::earth_beta;
	}
}


/**************************************************************************
*								Meshes
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	if(!ctx->world().component<StaticMeshComponent>(id)) {
		return;
	}
	if(!ImGui::CollapsingHeader("Static mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	const auto static_mesh = [ctx, id]() -> StaticMeshComponent* { return ctx->world().component<StaticMeshComponent>(id); };
	{
		// material
		if(imgui::asset_selector(ctx, static_mesh()->material().id(), AssetType::Material, "Material")) {
			ctx->ui().add<AssetSelector>(AssetType::Material)->set_selected_callback(
				[=](AssetId asset) {
					if(const auto material = ctx->loader().load<Material>(asset)) {
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
					if(const auto mesh = ctx->loader().load<StaticMesh>(asset)) {
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
*								Transformable
**************************************************************************/

editor_widget_draw_func(ContextPtr ctx, ecs::EntityId id) {
	TransformableComponent* component = ctx->world().component<TransformableComponent>(id);
	if(!component) {
		return;
	}

	if(!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	{
		auto [pos, rot, scale] = component->transform().decompose();

		// position
		{
			ImGui::InputFloat3("Position", pos.data(), "%.2f");
		}

		// rotation
		{
			auto dedouble_quat = [](math::Quaternion<> q) {
				return q.x() < 0.0f ? -q.as_vec() : q.as_vec();
			};
			auto is_same_angle = [&](math::Vec3 a, math::Vec3 b) {
				auto qa = math::Quaternion<>::from_euler(a * math::to_rad(1.0f));
				auto qb = math::Quaternion<>::from_euler(b * math::to_rad(1.0f));
				return (dedouble_quat(qa) - dedouble_quat(qb)).length2() < 0.0001f;
			};

			const math::Vec3 base_euler = rot.to_euler() * math::to_deg(1.0f);
			math::Vec3 euler = base_euler;
			/*if(is_same_angle(euler, prev_euler)) {
				euler = prev_euler;
			}*/

			const float speed = 1.0f;
			ImGui::DragFloat("Yaw", &euler[math::Quaternion<>::YawIndex], speed, -180.0f, 180.0f, "%.2f");
			ImGui::DragFloat("Pitch", &euler[math::Quaternion<>::PitchIndex], speed, -180.0f, 180.0f, "%.2f");
			ImGui::DragFloat("Roll", &euler[math::Quaternion<>::RollIndex], speed, -180.0f, 180.0f, "%.2f");

			if(!is_same_angle(euler, base_euler)) {
				rot = math::Quaternion<>::from_euler(euler * math::to_rad(1.0f));
			}
		}

		component->transform() = math::Transform<>(pos, rot, scale);
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

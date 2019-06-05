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
#ifndef EDITOR_COMPONENTS_WIDGETS_H
#define EDITOR_COMPONENTS_WIDGETS_H

#include "ComponentTraits.h"

#include <editor/context/EditorContext.h>
#include <editor/widgets/AssetSelector.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <yave/utils/color.h>

#include <imgui/yave_imgui.h>

namespace editor {

template<typename T>
void widget(ContextPtr, ecs::EntityId) {
}





/**************************************************************************
*								Editor
**************************************************************************/

template<>
void widget<EditorComponent>(ContextPtr ctx, ecs::EntityId id) {
	if(!ImGui::CollapsingHeader("Entity")) {
		return;
	}

	EditorComponent* component = ctx->world().component<EditorComponent>(id);

	const core::String& name = component->name();

	std::array<char, 1024> name_buffer;
	std::fill(name_buffer.begin(), name_buffer.end(), 0);
	std::copy(name.begin(), name.end(), name_buffer.begin());

	if(ImGui::InputText("Name", name_buffer.begin(), name_buffer.size())) {
		component->set_name(name_buffer.begin());
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

template<>
void widget<PointLightComponent>(ContextPtr ctx, ecs::EntityId id) {
	if(!ImGui::CollapsingHeader("Point light")) {
		return;
	}

	PointLightComponent* light = ctx->world().component<PointLightComponent>(id);

	light_widget(light);
	ImGui::DragFloat("Radius", &light->radius(), 1.0f, 0.0f, std::numeric_limits<float>::max());
}

template<>
void widget<DirectionalLightComponent>(ContextPtr ctx, ecs::EntityId id) {
	if(!ImGui::CollapsingHeader("Directional light")) {
		return;
	}

	DirectionalLightComponent* light = ctx->world().component<DirectionalLightComponent>(id);

	light_widget(light);

	{
		ImGui::BeginGroup();
		ImGui::InputFloat3("Direction", light->direction().data(), "%.2f");
		ImGui::EndGroup();
	}

}






/**************************************************************************
*								Meshes
**************************************************************************/

template<>
void widget<StaticMeshComponent>(ContextPtr ctx, ecs::EntityId id) {
	auto clean_name = [=](auto&& n) { return ctx->asset_store().filesystem()->filename(n); };
	if(!ImGui::CollapsingHeader("Static mesh")) {
		return;
	}

	Y_TODO(use ECS to ensure that we dont modify a deleted object)

	StaticMeshComponent* static_mesh = ctx->world().component<StaticMeshComponent>(id);
	{
		// material
		{
			core::String name = ctx->asset_store().name(static_mesh->material().id()).map(clean_name).unwrap_or("No material");
			if(ImGui::Button(ICON_FA_FOLDER_OPEN "###material")) {
				ctx->ui().add<AssetSelector>(AssetType::Material)->set_selected_callback(
							[=](AssetId asset) {
					if(auto material = ctx->loader().load<Material>(asset)) {
						static_mesh->material() = material.unwrap();
					}
					return true;
				});
			}
			ImGui::SameLine();
			ImGui::InputText("Material", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
		}

		// mesh
		{
			core::String name = ctx->asset_store().name(static_mesh->mesh().id()).map(clean_name).unwrap_or("No mesh");
			if(ImGui::Button(ICON_FA_FOLDER_OPEN "###mesh")) {
				ctx->ui().add<AssetSelector>(AssetType::Mesh)->set_selected_callback(
							[=](AssetId asset) {
					if(auto mesh = ctx->loader().load<StaticMesh>(asset)) {
						static_mesh->mesh() = mesh.unwrap();
					}
					return true;
				});
			}
			ImGui::SameLine();
			ImGui::InputText("Mesh", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
		}
	}
}





/**************************************************************************
*								Transformable
**************************************************************************/

template<>
void widget<TransformableComponent>(ContextPtr ctx, ecs::EntityId id) {
	if(!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	TransformableComponent* component = ctx->world().component<TransformableComponent>(id);
	{
		math::Vec3 prev_euler; // this should be stable
		auto [pos, rot, scale] = component->transform().decompose();

		// position
		{
			ImGui::BeginGroup();
			ImGui::InputFloat3("Position", pos.data(), "%.2f");
			ImGui::EndGroup();
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

			math::Vec3 euler = rot.to_euler() * math::to_deg(1.0f);
			if(is_same_angle(euler, prev_euler)) {
				euler = prev_euler;
			}

			float speed = 1.0f;
			ImGui::BeginGroup();
			ImGui::DragFloat("Yaw", &euler[math::Quaternion<>::YawIndex], speed, -180.0f, 180.0f, "%.2f");
			ImGui::DragFloat("Pitch", &euler[math::Quaternion<>::PitchIndex], speed, -180.0f, 180.0f, "%.2f");
			ImGui::DragFloat("Roll", &euler[math::Quaternion<>::RollIndex], speed, -180.0f, 180.0f, "%.2f");
			ImGui::EndGroup();

			if(!is_same_angle(euler, prev_euler)) {
				prev_euler = euler;
				rot = math::Quaternion<>::from_euler(euler * math::to_rad(1.0f));
			}
		}

		component->transform() = math::Transform<>(pos, rot, scale);
	}
}

}

#endif // EDITOR_COMPONENTS_WIDGETS_H

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
#include <yave/objects/StaticMeshInstance.h>
#include <yave/ecs/EntityWorld.h>

#include <imgui/yave_imgui.h>

namespace editor {

template<typename T>
void widget(ContextPtr, ecs::EntityId) {
}

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

template<>
void widget<LightComponent>(ContextPtr ctx, ecs::EntityId id) {
	if(!ImGui::CollapsingHeader("Light component")) {
		return;
	}

	LightComponent* component = ctx->world().component<LightComponent>(id);
	Light& light = component->light();

	int color_flags = ImGuiColorEditFlags_NoSidePreview |
					  // ImGuiColorEditFlags_NoSmallPreview |
					  ImGuiColorEditFlags_NoAlpha |
					  ImGuiColorEditFlags_Float |
					  ImGuiColorEditFlags_InputRGB;

	math::Vec4 color(light.color(), 1.0f);
	if(ImGui::ColorButton("Color", color, color_flags)) {
		ImGui::OpenPopup("Color");
	}
	ImGui::SameLine();
	ImGui::Text("Light color");

	if(ImGui::BeginPopup("Color")) {
		ImGui::ColorPicker3("##lightpicker", light.color().begin(), color_flags);

		ImGui::SameLine();
		ImGui::BeginGroup();
		ImGui::Text("temperature slider should be here");
		ImGui::EndGroup();

		ImGui::EndPopup();
	}

	ImGui::DragFloat("Intensity", &light.intensity());
	ImGui::DragFloat("Radius", &light.radius());
}

template<>
void widget<RenderableComponent>(ContextPtr ctx, ecs::EntityId id) {
	RenderableComponent* component = ctx->world().component<RenderableComponent>(id);
	StaticMeshInstance* instance = dynamic_cast<StaticMeshInstance*>(component->get());
	if(!instance) {
		return;
	}

	if(!ImGui::CollapsingHeader("Static mesh")) {
		return;
	}

	Y_TODO(use ECS to ensure that we dont modify a deleted object)

	auto clean_name = [=](auto&& n) { return ctx->asset_store().filesystem()->filename(n); };

	// material
	{
		core::String name = ctx->asset_store().name(instance->material().id()).map(clean_name).unwrap_or("No material");
		if(ImGui::Button(ICON_FA_FOLDER_OPEN)) {
			ctx->ui().add<AssetSelector>(AssetType::Material)->set_selected_callback(
				[=](AssetId asset) {
					if(auto material = ctx->loader().load<Material>(asset)) {
						instance->material() = material.unwrap();
					}
					return true;
				});
		}
		ImGui::SameLine();
		ImGui::InputText("Material", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
	}

	// mesh
	{
		core::String name = ctx->asset_store().name(instance->mesh().id()).map(clean_name).unwrap_or("No mesh");
		if(ImGui::Button(ICON_FA_FOLDER_OPEN)) {
			ctx->ui().add<AssetSelector>(AssetType::Mesh)->set_selected_callback(
				[=](AssetId asset) {
					if(auto mesh = ctx->loader().load<StaticMesh>(asset)) {
						instance->mesh() = mesh.unwrap();
					}
					return true;
				});
		}
		ImGui::SameLine();
		ImGui::InputText("Mesh", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
	}
}

}

#endif // EDITOR_COMPONENTS_WIDGETS_H

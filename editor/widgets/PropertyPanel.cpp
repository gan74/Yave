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

#include "PropertyPanel.h"

#include <editor/context/EditorContext.h>
#include <editor/widgets/AssetSelector.h>

#include <imgui/imgui.h>

namespace editor {

PropertyPanel::PropertyPanel(ContextPtr cptr) :
		Widget(ICON_FA_WRENCH " Properties"),
		ContextLinked(cptr) {
	set_closable(false);
}

/*bool PropertyPanel::is_visible() const {
	return UiElement::is_visible() && context()->selection().selected();
}*/

void PropertyPanel::paint(CmdBufferRecorder& recorder, const FrameToken& token) {
	Widget::paint(recorder, token);

	/*if(Transformable* selected = context()->selection().selected()) {
		auto end_pos = context()->scene().to_window_pos(selected->position());
		auto start_pos = position() + math::Vec2(0.0f, 12.0f);

		u32 color = ImGui::GetColorU32(ImGuiCol_Text) | 0xFF000000;

		// ImGui::GetWindowDrawList()->AddLine(start_pos, end_pos, color);

		auto point = math::Vec2(std::copysign(128.0f, end_pos.x() - start_pos.x()), 0.0f);
		ImGui::GetWindowDrawList()->AddBezierCurve(start_pos, start_pos + point, end_pos - point, end_pos, color, 2.0f);
	}*/
}

void PropertyPanel::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	if(!context()->selection().transformable()) {
		return;
	}

	Transformable* sel = context()->selection().transformable();
	auto [pos, rot, scale] = sel->transform().decompose();

	if(ImGui::CollapsingHeader("Position", ImGuiTreeNodeFlags_DefaultOpen)) {
		float step = 1.0f;
		float big_step = 15.0f;

		ImGui::BeginGroup();
		ImGui::InputFloat("X", &pos.x(), step, big_step);
		ImGui::InputFloat("Y", &pos.y(), step, big_step);
		ImGui::InputFloat("Z", &pos.z(), step, big_step);
		ImGui::EndGroup();
	}

	if(ImGui::CollapsingHeader("Rotation", ImGuiTreeNodeFlags_DefaultOpen)) {
		math::Vec3 euler = rot.to_euler() * math::to_deg(1.0f);
		for(auto& a : euler) {
			// remove -0.0
			if(a == 0.0f) {
				a = 0.0f;
			}
			if(a == -180.0f) {
				a = 180.0f;
			}
		}

		bool angle_changed = false;
		float step = 1.0f;
		float big_step = 15.0f;

		ImGui::BeginGroup();
		angle_changed |= ImGui::InputFloat("Pitch", &euler[math::Quaternion<>::PitchIndex], step, big_step);
		angle_changed |= ImGui::InputFloat("Yaw", &euler[math::Quaternion<>::YawIndex], step, big_step);
		angle_changed |= ImGui::InputFloat("Roll", &euler[math::Quaternion<>::RollIndex], step, big_step);
		ImGui::EndGroup();

		// avoid recomputing angle (not always stable in euler space)
		if(angle_changed) {
			rot = math::Quaternion<>::from_euler(euler * math::to_rad(1.0f));
		}
	}

	if(Renderable* renderable = context()->selection().renderable()) {
		if(StaticMeshInstance* mesh = dynamic_cast<StaticMeshInstance*>(renderable)) {

			if(ImGui::CollapsingHeader("Static mesh", ImGuiTreeNodeFlags_DefaultOpen)) {
				if(ImGui::Button(fmt("%", mesh->material().id()).data())) {
					context()->ui().add<AssetSelector>(AssetType::Material)->set_selected_callback(
						[=, ctx = context()](AssetId id) {
#warning Use ECS to ensure that we dont modify a deleted object
							if(auto material = ctx->loader().load<Material>(id)) {
								mesh->material() = material.unwrap();
							}
							return true;
						});
				}
			}
		}
	}


	if(Light* light = context()->selection().light()) {
		int flags = ImGuiColorEditFlags_NoSidePreview |
					// ImGuiColorEditFlags_NoSmallPreview |
					ImGuiColorEditFlags_NoAlpha |
					ImGuiColorEditFlags_Float |
					ImGuiColorEditFlags_InputRGB;

		if(ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
			math::Vec4 color(light->color(), 1.0f);
			if(ImGui::ColorButton("Color", color, flags/*, ImVec2(40, 40)*/)) {
				ImGui::OpenPopup("Color");
			}
			ImGui::SameLine();
			ImGui::Text("Light color");
		}

		if(ImGui::BeginPopup("Color")) {
			ImGui::ColorPicker3("##lightpicker", light->color().begin(), flags);

			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::Text("temperature slider should be here");
			ImGui::EndGroup();

			ImGui::EndPopup();
		}

		ImGui::InputFloat("Intensity", &light->intensity(), 1.0f, 10.0f);
		ImGui::InputFloat("Radius", &light->radius(), 1.0f, 10.0f);


	}

	sel->transform() = math::Transform<>(pos, rot, scale);

	//ImGui::Text("Position: %f, %f, %f", sel->position().x(), sel->position().y(), sel->position().z());

}

}

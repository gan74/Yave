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
#include <editor/components/ComponentTraits.h>

#include <imgui/yave_imgui.h>

namespace editor {

PropertyPanel::PropertyPanel(ContextPtr cptr) :
		Widget(ICON_FA_WRENCH " Properties"),
		ContextLinked(cptr) {

	set_closable(false);
}

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
	if(!context()->selection().has_selected_entity()) {
		return;
	}

	ecs::EntityWorld& world = context()->world();
	ecs::EntityId id = context()->selection().selected_entity();

	for(const auto& p : world.component_containers()) {
		if(p.second->has(id)) {
			component_widget(p.first, context(), id);
		}
	}

	//ImGui::Text("Position: %f, %f, %f", sel->position().x(), sel->position().y(), sel->position().z());*/
}

void PropertyPanel::transformable_panel(Transformable& transformable) {
	if(!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		return;
	}

	auto [pos, rot, scale] = transformable.transform().decompose();

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
		if(is_same_angle(euler, _euler)) {
			euler = _euler;
		}

		float speed = 1.0f;
		ImGui::BeginGroup();
		ImGui::DragFloat("Yaw", &euler[math::Quaternion<>::YawIndex], speed, -180.0f, 180.0f, "%.2f");
		ImGui::DragFloat("Pitch", &euler[math::Quaternion<>::PitchIndex], speed, -180.0f, 180.0f, "%.2f");
		ImGui::DragFloat("Roll", &euler[math::Quaternion<>::RollIndex], speed, -180.0f, 180.0f, "%.2f");
		ImGui::EndGroup();

		if(!is_same_angle(euler, _euler)) {
			_euler = euler;
			rot = math::Quaternion<>::from_euler(euler * math::to_rad(1.0f));
		}
	}

	transformable.transform() = math::Transform<>(pos, rot, scale);
}

}

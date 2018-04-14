/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include "Gizmo.h"

#include <editor/EditorContext.h>

#include <imgui/imgui.h>

namespace editor {

static constexpr float gizmo_width = 3.0f;
static constexpr float gizmo_size = 0.15f;
static constexpr u32 gizmo_alpha = 0xC0000000;

static bool is_clicked() {
#warning gizmo can be grabbed through other ui elements
	return ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive();
}

static bool is_clicking(math::Vec2 cursor, const math::Vec2& vec) {
	float len = vec.length();
	math::Vec2 n = vec / len;
	float d = cursor.dot(n);

	if(d > len || d < 0.0f) {
		return false;
	}

	float d_2 = std::fabs(cursor.length2() - (d * d));
	return d_2 < (2.0f * gizmo_width * gizmo_width);
}

static math::Vec2 to_screen_pos(const math::Matrix4<>& view_proj, const math::Vec3& pos) {
	auto h_pos = view_proj * math::Vec4(pos, 1.0f);
	return (h_pos.to<2>() / h_pos.w()) * 0.5f + 0.5f;
}



Gizmo::Gizmo(ContextPtr cptr) : Gadget("Gizmo"), ContextLinked(cptr) {
}

void Gizmo::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	if(!context()->selected()) {
		return;
	}

	math::Vec3 cam_fwd = context()->scene_view()->camera().forward();
	math::Vec3 cam_pos = context()->scene_view()->camera().position();
	math::Matrix4<> view_proj = context()->scene_view()->camera().viewproj_matrix();
	math::Vec3 obj_pos = context()->selected()->transform().position();

	if(cam_fwd.dot(obj_pos - cam_pos) < 0.0f) {
		return;
	}

	auto projected = (view_proj * math::Vec4(obj_pos, 1.0f));
	auto perspective = gizmo_size * projected.w();

	math::Vec2 viewport = ImGui::GetWindowSize();
	math::Vec2 offset = ImGui::GetWindowPos();

	auto center = to_screen_pos(view_proj, obj_pos) * viewport + offset;

	math::Vec3 basis[] = {{perspective, 0.0f, 0.0f}, {0.0f, perspective, 0.0f}, {0.0f, 0.0f, perspective}};
	math::Vec2 axis[] = {
			to_screen_pos(view_proj, obj_pos + basis[0]) * viewport + offset,
			to_screen_pos(view_proj, obj_pos + basis[1]) * viewport + offset,
			to_screen_pos(view_proj, obj_pos + basis[2]) * viewport + offset
		};

	usize sorted[] = {0, 1, 2};
	sort(std::begin(sorted), std::end(sorted), [&](usize a, usize b) {
		return basis[a].dot(cam_fwd) > basis[b].dot(cam_fwd);
	});

	for(usize i = 0; i != 3; ++i) {
		usize sorted_index = sorted[i];
		u32 color = gizmo_alpha | (0xFF << (8 * sorted_index));
		ImGui::GetWindowDrawList()->AddLine(center, axis[sorted_index], color, gizmo_width);
	}
	ImGui::GetWindowDrawList()->AddCircleFilled(center, 1.5f * gizmo_width, 0x00FFFFFF | gizmo_alpha);


	auto project_mouse = [=]{
		auto inv_matrix = context()->scene_view()->camera().inverse_matrix();

		math::Vec2 ndc = ((math::Vec2(ImGui::GetIO().MousePos) - offset) / viewport) * 2.0f - 1.0f;
		math::Vec4 h_world = inv_matrix * math::Vec4(ndc, 0.5f, 1.0f);
		math::Vec3 world = h_world.to<3>() / h_world.w();

		math::Vec3 ray = (world - cam_pos).normalized();
		float dist = (obj_pos - cam_pos).length();

		return cam_pos + ray * dist;
	};

	if(is_clicked()) {
		math::Vec2 cursor = math::Vec2(ImGui::GetIO().MousePos) - center;
		_dragging_mask = 0;
		_dragging_offset = obj_pos - project_mouse();
		for(usize i = 0; i != 3; ++i) {
			if(is_clicking(cursor, axis[i] - center)) {
				_dragging_mask |= (1 << i);
			}
		}
	} else if(!ImGui::IsMouseDown(0)) {
		_dragging_mask = 0;
	}


	if(_dragging_mask) {
		auto new_pos = project_mouse() + _dragging_offset;
		for(usize i = 0; i != 3; ++i) {
			if(_dragging_mask & (1 << i)) {
				context()->selected()->position()[i] = new_pos[i];
			}
		}
	}
}


}

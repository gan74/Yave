/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <editor/EditorContext.h>

#include <imgui/imgui.h>

namespace editor {

PropertyPanel::PropertyPanel(ContextPtr cptr) : Dock("Properties"), ContextLinked(cptr) {
}


void PropertyPanel::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	if(!context()->selected) {
		return;
	}


	Transformable* sel = context()->selected;
	auto [pos, rot, scale] = sel->transform().decompose();
	math::Vec3 euler = rot.to_euler();

	ImGui::InputFloat3("Position", pos.begin());

	ImGui::InputFloat3("Rotation", euler.begin());

	sel->transform() = math::Transform<>(pos, math::Quaternion<>::from_euler(euler), scale);

	//ImGui::Text("Position: %f, %f, %f", sel->position().x(), sel->position().y(), sel->position().z());

}

}

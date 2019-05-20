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

#include "SettingsPanel.h"

#include <editor/context/EditorContext.h>
#include <editor/renderers/ImGuiRenderer.h>

#include <imgui/yave_imgui.h>

namespace editor {

static void keybox(const char* name, Key& key) {
	char k[2] = {char(key), 0};

	ImGui::PushItemWidth(24);
	ImGuiInputTextCallback callback = [](ImGuiInputTextCallbackData* data) { data->CursorPos = 0; return 0; };
	ImGui::InputText(name, k, sizeof(k), ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AlwaysInsertMode | ImGuiInputTextFlags_CallbackAlways, callback);

	if(std::isalpha(k[0])) {
		key = Key(k[0]);
	}

	ImGui::PopItemWidth();
}

SettingsPanel::SettingsPanel(ContextPtr cptr) : Widget("Settings"), ContextLinked(cptr) {
}

void SettingsPanel::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	int flags = ImGuiTreeNodeFlags_DefaultOpen;

	/*if(ImGui::CollapsingHeader("UI", flags)) {
		std::array<const char*, 3> style_names = {"Yave", "Corporate", "Corporate 3D"};
		if(ImGui::BeginCombo("Style", style_names[usize(context()->settings().ui().style)])) {
			for(usize i = 0; i != style_names.size(); ++i) {
				if(ImGui::Selectable(style_names[i])) {
					context()->settings().ui().style = ImGuiRenderer::Style(i);
				}
			}
			ImGui::EndCombo();
		}
	}*/

	if(ImGui::CollapsingHeader("Camera", flags)) {
		keybox("Forward", context()->settings().camera().move_forward);
		keybox("Backward", context()->settings().camera().move_backward);
		keybox("Left", context()->settings().camera().move_left);
		keybox("Right", context()->settings().camera().move_right);

		ImGui::Separator();

		ImGui::SliderFloat("Sensitivity", &context()->settings().camera().sensitivity, 0.1f, 10.0f, "%.1f", 2.0f);
	}
}

}

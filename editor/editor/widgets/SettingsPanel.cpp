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

#include "SettingsPanel.h"

#include <editor/EditorContext.h>

#include <imgui/imgui.h>

namespace editor {

static void keybox(const char* name, Key& key) {
	char k[2] = {char(key), 0};

	ImGuiTextEditCallback callback = [](ImGuiTextEditCallbackData* data) { data->CursorPos = 0; return 0; };
	ImGui::InputText(name, k, sizeof(k), ImGuiInputTextFlags_CharsUppercase | ImGuiInputTextFlags_AlwaysInsertMode | ImGuiInputTextFlags_CallbackAlways, callback);

	if(std::isalpha(k[0])) {
		key = Key(k[0]);
	}
}

SettingsPanel::SettingsPanel(ContextPtr cptr) : Dock("Settings"), ContextLinked(cptr) {
}

void SettingsPanel::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	int flags = ImGuiTreeNodeFlags_DefaultOpen;

	if(ImGui::CollapsingHeader("Camera", flags)) {
		keybox("Forward", context()->key_settings.move_forward);
		keybox("Backward", context()->key_settings.move_backward);
		keybox("Left", context()->key_settings.move_left);
		keybox("Right", context()->key_settings.move_right);
	}
}

}

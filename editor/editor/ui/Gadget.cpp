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

#include "Gadget.h"

#include <imgui/imgui.h>

namespace editor {

Gadget::Gadget(const char* title) : UiElement(title) {
}

void Gadget::paint(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	if(!is_visible()) {
		return;
	}

	// this breaks everthing that relies on getting focus (like popups)
	// ImGui::SetNextWindowFocus();

	ImGui::SetNextWindowSize(ImGui::GetWindowSize());
	ImGui::SetNextWindowPos(ImGui::GetWindowPos());
	ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
	ImU32 flags = ImGuiWindowFlags_NoTitleBar |
				  ImGuiWindowFlags_NoResize |
				  ImGuiWindowFlags_NoScrollbar |
				  ImGuiWindowFlags_NoInputs |
				  ImGuiWindowFlags_NoSavedSettings |
				  ImGuiWindowFlags_NoFocusOnAppearing |
				  ImGuiWindowFlags_NoBringToFrontOnFocus |
				  0;

	if(ImGui::BeginChild(_title, ImGui::GetWindowSize(), &_visible, flags)) {
		paint_ui(recorder, token);
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

}

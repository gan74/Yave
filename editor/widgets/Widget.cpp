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

#include "Widget.h"

#include <imgui/imgui.h>

namespace editor {

static ImU32 setup_flags(Widget::Flags f) {
	if(f == Widget::NoWindow) {
		ImGuiIO& io = ImGui::GetIO();
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);
		return ImGuiWindowFlags_NoTitleBar |
			   ImGuiWindowFlags_NoResize |
			   ImGuiWindowFlags_NoScrollbar |
			   ImGuiWindowFlags_NoInputs |
			   ImGuiWindowFlags_NoSavedSettings |
			   ImGuiWindowFlags_NoFocusOnAppearing |
			   ImGuiWindowFlags_NoBringToFrontOnFocus;
	}
	return false;
}

static void cleanup_flags(Widget::Flags f) {
	if(f == Widget::NoWindow) {
		ImGui::PopStyleColor();
	}
}

Widget::Widget(const char* title, Flags flags) : _title(title), _flags(flags) {
}

Widget::~Widget() {
}

void Widget::paint() {
	if(!is_visible()) {
		return;
	}

	auto imgui_flags = setup_flags(_flags);
	{
		ImGui::Begin(_title, &_visible, imgui_flags);

		paint_ui();

		ImGui::End();
	}
	cleanup_flags(_flags);
}

void Widget::show() {
	_visible = true;
}

bool Widget::is_visible() const {
	return _visible;
}

const char* Widget::title() const {
	return _title;
}

}

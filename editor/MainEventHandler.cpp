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
#include "MainEventHandler.h"

#include <imgui/imgui.h>

namespace editor {

MainEventHandler::MainEventHandler() {
}

MainEventHandler::~MainEventHandler() {
}

void MainEventHandler::mouse_moved(const math::Vec2i& pos) {
	ImGuiIO& io = ImGui::GetIO();
	io.MousePos = ImVec2(pos.x(), pos.y());
}

void MainEventHandler::mouse_pressed(const math::Vec2i& pos, MouseButton button) {
	mouse_moved(pos);
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[button != MouseButton::LeftButton] = true;
}

void MainEventHandler::mouse_released(const math::Vec2i& pos, MouseButton button) {
	mouse_moved(pos);
	ImGuiIO& io = ImGui::GetIO();
	io.MouseDown[button != MouseButton::LeftButton] = false;
}

void MainEventHandler::char_input(u32 character) {
	ImGuiIO& io = ImGui::GetIO();
	u32 utf8[2] = {character, 0};
	io.AddInputCharactersUTF8(reinterpret_cast<const char*>(utf8));
}


}

/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <external/imgui/yave_imgui.h>

#include <y/utils/log.h>

#include <array>

namespace editor {

static void set_key_bindings() {
    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab]         = int(Key::Tab);
    io.KeyMap[ImGuiKey_LeftArrow]   = int(Key::Left);
    io.KeyMap[ImGuiKey_RightArrow]  = int(Key::Right);
    io.KeyMap[ImGuiKey_UpArrow]     = int(Key::Up);
    io.KeyMap[ImGuiKey_DownArrow]   = int(Key::Down);
    io.KeyMap[ImGuiKey_PageUp]      = int(Key::PageUp);
    io.KeyMap[ImGuiKey_PageDown]    = int(Key::PageDown);
    io.KeyMap[ImGuiKey_Home]        = int(Key::Home);
    io.KeyMap[ImGuiKey_End]         = int(Key::End);
    io.KeyMap[ImGuiKey_Delete]      = int(Key::Delete);
    io.KeyMap[ImGuiKey_Backspace]   = int(Key::Backspace);
    io.KeyMap[ImGuiKey_Enter]       = int(Key::Enter);
    io.KeyMap[ImGuiKey_Escape]      = int(Key::Escape);

    io.KeyMap[ImGuiKey_A]           = int(Key::Max);
    io.KeyMap[ImGuiKey_C]           = io.KeyMap[ImGuiKey_A] + 1;
    io.KeyMap[ImGuiKey_V]           = io.KeyMap[ImGuiKey_A] + 2;
    io.KeyMap[ImGuiKey_X]           = io.KeyMap[ImGuiKey_A] + 3;
    io.KeyMap[ImGuiKey_Y]           = io.KeyMap[ImGuiKey_A] + 4;
    io.KeyMap[ImGuiKey_Z]           = io.KeyMap[ImGuiKey_A] + 5;
}


MainEventHandler::MainEventHandler() {
    set_key_bindings();
}

MainEventHandler::~MainEventHandler() {
}

void MainEventHandler::mouse_moved(const math::Vec2i& pos) {
    ImGui::GetIO().MousePos = ImVec2(pos.x(), pos.y());
}

void MainEventHandler::mouse_pressed(const math::Vec2i& pos, MouseButton button) {
    mouse_moved(pos);
    ImGui::GetIO().MouseDown[usize(button)] = true;
}

void MainEventHandler::mouse_released(const math::Vec2i& pos, MouseButton button) {
    mouse_moved(pos);
    ImGui::GetIO().MouseDown[usize(button)] = false;
}

void MainEventHandler::mouse_wheel(i32 vdelta, i32 hdelta) {
    ImGui::GetIO().MouseWheel += vdelta;
    ImGui::GetIO().MouseWheelH += hdelta;
}

void MainEventHandler::char_input(u32 character) {
    ImGui::GetIO().AddInputCharacter(ImWchar(character));
}

void MainEventHandler::key_pressed(Key key) {
    key_event(key, true);
}

void MainEventHandler::key_released(Key key) {
    key_event(key, false);
}


void MainEventHandler::key_event(Key key, bool pressed) {
    auto& io = ImGui::GetIO();

    io.KeysDown[u32(key)] = pressed;

    switch(key) {
        case Key::Ctrl:
            io.KeyCtrl = pressed;
        break;

        case Key::Alt:
            io.KeyAlt = pressed;
        break;

        default:
        break;
    }

    const std::array shortcuts = {Key::A, Key::C, Key::V, Key::X, Key::Y, Key::Z};
    for(usize i = 0; i != shortcuts.size(); ++i) {
        bool& down = io.KeysDown[io.KeyMap[ImGuiKey_A + i]];
        if(shortcuts[i] == key) {
            down = pressed;
        }
        down &= io.KeyCtrl;
    }

}

}


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

#include "ImGuiPlatform.h"

#include <y/io2/File.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

static ImGuiPlatform* get_platform(ImGuiViewport* vp) {
    y_debug_assert(vp->PlatformUserData);
    return static_cast<ImGuiPlatform*>(vp->PlatformUserData);
}

static Window* get_window(ImGuiViewport* vp) {
    y_debug_assert(vp->PlatformHandle);
    return static_cast<Window*>(vp->PlatformHandle);
}


static void create_window_callback(ImGuiViewport* vp) {
    vp->PlatformHandle = get_platform(vp)->create_window(vp->Size);
}

static void close_window_callback(ImGuiViewport* vp) {
    get_window(vp)->close();
}

static void show_window_callback(ImGuiViewport* vp) {
    get_window(vp)->show();
}

ImGuiPlatform::ImGuiPlatform() {
    ImGui::CreateContext();

    {
        auto& io = ImGui::GetIO();

        io.IniFilename = "editor.ini";
        io.LogFilename = "editor_logs.txt";
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable/* | ImGuiConfigFlags_ViewportsEnable*/;
        io.ConfigDockingWithShift = false;

        if(io2::File::open("../editor.ini").is_ok()) {
            io.IniFilename = "../editor.ini";
        }

        y_always_assert(io.BackendPlatformUserData == nullptr, "ImGui already has a platform backend.");
        io.BackendPlatformUserData = this;
    }


    // {
    //     auto& platform = ImGui::GetPlatformIO();
    //     platform.Platform_CreateWindow      = create_window_callback;
    //     platform.Platform_DestroyWindow     = close_window_callback;
    //     platform.Platform_ShowWindow        = show_window_callback;
    // }
}

Window* ImGuiPlatform::create_window(const math::Vec2ui& size, const core::String& name) {
    _windows << std::make_unique<Window>(size, name);
    Window* win = _windows.last().get();

    win->show();

    return win;
}

}

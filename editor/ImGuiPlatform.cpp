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

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/window/Monitor.h>
#include <yave/window/EventHandler.h>

#include <y/io2/File.h>
#include <y/utils/log.h>

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

static void set_window_pos_callback(ImGuiViewport* vp, ImVec2 pos) {
    get_window(vp)->set_position(pos);
}

static void set_window_size_callback(ImGuiViewport* vp, ImVec2 size) {
    get_window(vp)->set_size(size);
}

static ImVec2 get_window_pos_callback(ImGuiViewport* vp) {
    return get_window(vp)->position();
}

static ImVec2 get_window_size_callback(ImGuiViewport* vp) {
    return get_window(vp)->size();
}



static void window_set_title_callback(ImGuiViewport* vp, const char* title) {
    get_window(vp)->set_title(title);
}


struct ImGuiEventHandler : EventHandler{
    void mouse_moved(const math::Vec2i& pos) override {
        ImGui::GetIO().MousePos = ImVec2(pos.x(), pos.y());
    }

    void mouse_pressed(const math::Vec2i& pos, MouseButton button) override {
        mouse_moved(pos);
        ImGui::GetIO().MouseDown[usize(button)] = true;
    }

    void mouse_released(const math::Vec2i& pos, MouseButton button) override {
        mouse_moved(pos);
        ImGui::GetIO().MouseDown[usize(button)] = false;
    }

    void mouse_wheel(int delta) override {
        ImGui::GetIO().MouseWheel += delta;
    }

    void char_input(u32 character) override {
        ImGui::GetIO().AddInputCharacter(ImWchar(character));
    }

    void key_pressed(Key key) override {
        key_event(key, true);
    }

    void key_released(Key key) override {
        key_event(key, false);
    }


    void key_event(Key key, bool pressed) {
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
};








ImGuiPlatform::PlatformWindow::PlatformWindow(DevicePtr dptr, const math::Vec2ui& size, std::string_view title) :
        window(std::make_unique<Window>(size, title)),
        swapchain(std::make_unique<Swapchain>(dptr, window.get())) {

    window->set_event_handler(std::make_unique<ImGuiEventHandler>());
    window->show();
}


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



ImGuiPlatform::ImGuiPlatform(DevicePtr dptr, bool multi_viewport) : _main_window(dptr, {1280, 768}, "Yave") {

    ImGui::CreateContext();
    set_key_bindings();

    {
        auto& io = ImGui::GetIO();

        io.IniFilename = "editor.ini";
        io.LogFilename = "editor_logs.txt";
        if(io2::File::open("../editor.ini").is_ok()) {
            io.IniFilename = "../editor.ini";
        }

        io.ConfigDockingWithShift = false;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        if(multi_viewport) {
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
        }

        y_always_assert(io.BackendPlatformUserData == nullptr, "ImGui already has a platform backend.");
        io.BackendPlatformUserData = this;
    }


    if(multi_viewport) {
        auto& platform = ImGui::GetPlatformIO();
        platform.Platform_CreateWindow      = create_window_callback;
        platform.Platform_DestroyWindow     = close_window_callback;
        platform.Platform_ShowWindow        = show_window_callback;
        platform.Platform_SetWindowPos      = set_window_pos_callback;
        platform.Platform_SetWindowSize     = set_window_size_callback;
        platform.Platform_GetWindowPos      = get_window_pos_callback;
        platform.Platform_GetWindowSize     = get_window_size_callback;

        platform.Platform_SetWindowTitle    = window_set_title_callback;

        platform.Platform_SetWindowFocus = [](ImGuiViewport*) {};
        platform.Platform_GetWindowFocus = [](ImGuiViewport*) { return false; };
        platform.Platform_GetWindowMinimized = [](ImGuiViewport*) { return false; };

        platform.Monitors.clear();

        auto monitors = Monitor::monitors();
        std::sort(monitors.begin(), monitors.end(), [](const Monitor& a, const Monitor& b) { return b.is_primary < a.is_primary; });
        std::transform(monitors.begin(), monitors.end(), std::back_inserter(platform.Monitors), [](const Monitor& monitor) {
            ImGuiPlatformMonitor imgui_mon;
            imgui_mon.MainPos = monitor.position;
            imgui_mon.MainSize = monitor.size;
            imgui_mon.MainPos = monitor.position;
            imgui_mon.MainSize = monitor.size;
            return imgui_mon;
        });
    }


    _renderer = std::make_unique<ImGuiRenderer2>(dptr);
}

DevicePtr ImGuiPlatform::device() const {
    return _renderer->device();
}

void ImGuiPlatform::run() {
    while(_main_window.window->update()) {
        Swapchain* swapchain = _main_window.swapchain.get();

        if(swapchain && swapchain->is_valid()) {
            const FrameToken frame = swapchain->next_frame();
            CmdBufferRecorder recorder = create_disposable_cmd_buffer(device());

            ImGui::GetIO().DeltaTime = float(_frame_timer.reset().to_secs());
            ImGui::GetIO().DisplaySize = frame.image_view.size();

            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            ImGui::ShowMetricsWindow();

            ImGui::Render();

            {
                Framebuffer framebuffer(frame.image_view.device(), {frame.image_view});
                RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
                _renderer->render(pass);
            }

            // ImGui::UpdatePlatformWindows();
            // ImGui::RenderPlatformWindowsDefault();

            swapchain->present(frame, std::move(recorder), graphic_queue(device()));
        }
    }

}

Window* ImGuiPlatform::create_window(const math::Vec2ui& size, std::string_view title) {
    return _windows.emplace_back(device(), size, title).window.get();
}

}

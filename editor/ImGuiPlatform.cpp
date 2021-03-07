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

struct ImGuiEventHandler : EventHandler {
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


static void setup_key_bindings(ImGuiIO& io) {
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

static void setup_config_files(ImGuiIO& io) {
    io.IniFilename = "editor.ini";
    io.LogFilename = "editor_logs.txt";
    if(io2::File::open("../editor.ini").is_ok()) {
        io.IniFilename = "../editor.ini";
    }
}

static void setup_backend_flags(ImGuiIO& io, bool multi_viewport) {
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigDockingWithShift = false;

    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports | ImGuiBackendFlags_RendererHasViewports;
    if(multi_viewport) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
}

static void discover_monitors(ImGuiPlatformIO& platform) {
    platform.Monitors.clear();

    auto monitors = Monitor::monitors();
    std::sort(monitors.begin(), monitors.end(), [](const Monitor& a, const Monitor& b) { return b.is_primary < a.is_primary; });
    std::transform(monitors.begin(), monitors.end(), std::back_inserter(platform.Monitors), [](const Monitor& monitor) {
        ImGuiPlatformMonitor imgui_mon;
        imgui_mon.MainPos = monitor.position;
        imgui_mon.MainSize = monitor.size;
        imgui_mon.WorkPos = monitor.work_position;
        imgui_mon.WorkSize = monitor.work_size;
        return imgui_mon;
    });
}



ImGuiPlatform::PlatformWindow::PlatformWindow(ImGuiPlatform* parent, Window::Flags flags) :
        platform(parent),
        window({1280, 768}, "Window", flags),
        swapchain(platform->device(), &window) {

    window.set_event_handler(std::make_unique<ImGuiEventHandler>());
    window.show();
}

void ImGuiPlatform::PlatformWindow::render(ImGuiViewport* viewport) {
    DevicePtr dptr = platform->device();

    window.update();
    if(swapchain.size() != window.size()) {
        wait_all_queues(dptr);
        swapchain.reset();
    }

    if(swapchain.is_valid()) {
        const FrameToken token = swapchain.next_frame();
        CmdBufferRecorder recorder = create_disposable_cmd_buffer(dptr);

        {
            Framebuffer framebuffer(token.image_view.device(), {token.image_view});
            RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
            platform->_renderer->render(viewport->DrawData, pass);
        }

        swapchain.present(token, std::move(recorder), graphic_queue(dptr));
    }
}


ImGuiPlatform::ImGuiPlatform(DevicePtr dptr, bool multi_viewport) {
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();

    y_always_assert(io.BackendPlatformUserData == nullptr, "ImGui already has a platform backend.");
    io.BackendPlatformUserData = this;

    setup_key_bindings(io);
    setup_config_files(io);
    setup_backend_flags(io, multi_viewport);

    if(multi_viewport) {
        auto& platform = ImGui::GetPlatformIO();

        Y_TODO(do every frame)
        discover_monitors(platform);

        platform.Platform_CreateWindow      = [](ImGuiViewport* vp) {
            ImGuiPlatform* self = ImGuiPlatform::get_platform();
            vp->PlatformHandle = self->_windows.emplace_back(std::make_unique<PlatformWindow>(self)).get();
        };

        platform.Platform_DestroyWindow         = [](ImGuiViewport* vp) { get_window(vp)->close(); };
        platform.Platform_ShowWindow            = [](ImGuiViewport* vp) { get_window(vp)->show(); };
        platform.Platform_SetWindowPos          = [](ImGuiViewport* vp, ImVec2 pos) { log_msg(fmt("pos <<< %", math::Vec2(pos))); get_window(vp)->set_position(pos); };
        platform.Platform_SetWindowSize         = [](ImGuiViewport* vp, ImVec2 size) { get_window(vp)->set_size(size); };
        platform.Platform_GetWindowPos          = [](ImGuiViewport* vp) {  log_msg(fmt("pos >>> %", math::Vec2(get_window(vp)->position()))); return ImVec2(get_window(vp)->position()); };
        platform.Platform_GetWindowSize         = [](ImGuiViewport* vp) { return ImVec2(get_window(vp)->size()); };

        platform.Platform_SetWindowTitle        = [](ImGuiViewport* vp, const char* title) { get_window(vp)->set_title(title); };

        platform.Platform_SetWindowFocus        = [](ImGuiViewport*) {};
        platform.Platform_GetWindowFocus        = [](ImGuiViewport*) { return false; };
        platform.Platform_GetWindowMinimized    = [](ImGuiViewport*) { return false; };

        platform.Renderer_RenderWindow          = [](ImGuiViewport* vp, void*) { get_platform_window(vp)->render(vp); };

    }

    _renderer = std::make_unique<ImGuiRenderer2>(dptr);
    _main_window = std::make_unique<PlatformWindow>(this, Window::Resizable);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    viewport->PlatformHandle = _main_window.get();
}

DevicePtr ImGuiPlatform::device() const {
    return _renderer->device();
}

void ImGuiPlatform::run() {
    while(_main_window->window.update()) {
        Swapchain& swapchain = _main_window->swapchain;

        ImGui::GetIO().DeltaTime = float(_frame_timer.reset().to_secs());
        ImGui::GetIO().DisplaySize = _main_window->window.size();

        if(swapchain.is_valid()) {
            const FrameToken frame = swapchain.next_frame();
            CmdBufferRecorder recorder = create_disposable_cmd_buffer(device());

            ImGui::NewFrame();

            ImGui::ShowDemoWindow();
            //ImGui::ShowMetricsWindow();

            ImGui::Render();

            {
                Framebuffer framebuffer(frame.image_view.device(), {frame.image_view});
                RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
                _renderer->render(ImGui::GetDrawData(), pass);
            }

            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();

            swapchain.present(frame, std::move(recorder), graphic_queue(device()));
        }
    }
}


ImGuiPlatform* ImGuiPlatform::get_platform() {
    y_debug_assert(ImGui::GetIO().BackendPlatformUserData);
    return static_cast<ImGuiPlatform*>(ImGui::GetIO().BackendPlatformUserData);
}

Window* ImGuiPlatform::get_window(ImGuiViewport* vp) {
    return &get_platform_window(vp)->window;
}

ImGuiPlatform::PlatformWindow* ImGuiPlatform::get_platform_window(ImGuiViewport* vp) {
    y_debug_assert(vp->PlatformHandle);
    return static_cast<PlatformWindow*>(vp->PlatformHandle);
}

}

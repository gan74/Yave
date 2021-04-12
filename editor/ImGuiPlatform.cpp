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

#include <editor/utils/ui.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/window/Monitor.h>

#include <y/io2/File.h>
#include <y/utils/log.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

ImGuiPlatform* imgui_platform() {
    return ImGuiPlatform::instance();
}

struct ImGuiEventHandler : EventHandler {
    Window* window = nullptr;

    ImGuiEventHandler(Window* win) : window(win) {
    }

    void mouse_moved(const math::Vec2i& pos) override {
        // We can't use imgui::from_client_pos here since the ImGui window isn't set
        ImGui::GetIO().MousePos = ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable
            ? pos + window->position() : pos;
    }

    void mouse_pressed(const math::Vec2i& pos, MouseButton button) override {
        mouse_moved(pos);
        ImGui::GetIO().MouseDown[usize(button)] = true;
    }

    void mouse_released(const math::Vec2i& pos, MouseButton button) override {
        mouse_moved(pos);
        ImGui::GetIO().MouseDown[usize(button)] = false;
    }

    void mouse_wheel(i32 vdelta, i32 hdelta) override {
        ImGui::GetIO().MouseWheel += vdelta;
        ImGui::GetIO().MouseWheelH += hdelta;
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

static void setup_style(bool is_3D = true) {
    const float darkening = 1.0f;
    const auto dImVec4 = [=](float r, float g, float b, float a) { return ImVec4(r * darkening, g * darkening, b * darkening, a); };

    auto& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                   = dImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = dImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    //colors[ImGuiCol_ChildBg]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_ChildBg]                = dImVec4(0.00f, 0.00f, 0.00f, 0.10f);
    colors[ImGuiCol_WindowBg]               = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_Border]                 = dImVec4(0.12f, 0.12f, 0.12f, 0.71f);
    colors[ImGuiCol_BorderShadow]           = dImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_FrameBg]                = dImVec4(0.42f, 0.42f, 0.42f, 0.54f);
    colors[ImGuiCol_FrameBgHovered]         = dImVec4(0.42f, 0.42f, 0.42f, 0.40f);
    colors[ImGuiCol_FrameBgActive]          = dImVec4(0.56f, 0.56f, 0.56f, 0.67f);
    colors[ImGuiCol_TitleBg]                = dImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = dImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = dImVec4(0.17f, 0.17f, 0.17f, 0.90f);
    colors[ImGuiCol_MenuBarBg]              = dImVec4(0.335f, 0.335f, 0.335f, 1.000f);
    colors[ImGuiCol_ScrollbarBg]            = dImVec4(0.24f, 0.24f, 0.24f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = dImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = dImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = dImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_CheckMark]              = dImVec4(0.65f, 0.65f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = dImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = dImVec4(0.64f, 0.64f, 0.64f, 1.00f);
    colors[ImGuiCol_Button]                 = dImVec4(0.54f, 0.54f, 0.54f, 0.35f);
    colors[ImGuiCol_ButtonHovered]          = dImVec4(0.52f, 0.52f, 0.52f, 0.59f);
    colors[ImGuiCol_ButtonActive]           = dImVec4(0.76f, 0.76f, 0.76f, 1.00f);
    colors[ImGuiCol_Header]                 = dImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = dImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = dImVec4(0.76f, 0.76f, 0.76f, 0.77f);
    colors[ImGuiCol_Separator]              = dImVec4(0.000f, 0.000f, 0.000f, 0.137f);
    colors[ImGuiCol_SeparatorHovered]       = dImVec4(0.700f, 0.671f, 0.600f, 0.290f);
    colors[ImGuiCol_SeparatorActive]        = dImVec4(0.702f, 0.671f, 0.600f, 0.674f);
    colors[ImGuiCol_ResizeGrip]             = dImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = dImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = dImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines]              = dImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = dImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = dImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = dImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = dImVec4(0.73f, 0.73f, 0.73f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg]       = dImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = dImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = dImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = dImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = dImVec4(0.80f, 0.80f, 0.80f, 0.20f);

    style.PopupRounding     = 3;

    style.WindowPadding     = ImVec2(4, 4);
    style.FramePadding      = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(6, 2);

    style.ScrollbarSize     = 18;

    style.WindowBorderSize  = 1;
    style.ChildBorderSize   = 1;
    style.PopupBorderSize   = 1;
    style.FrameBorderSize   = is_3D;

    style.WindowRounding    = 3;
    style.ChildRounding     = 3;
    style.FrameRounding     = 3;
    style.ScrollbarRounding = 2;
    style.GrabRounding      = 3;

    style.TabBorderSize     = is_3D;
    style.TabRounding       = 3;

    colors[ImGuiCol_DockingEmptyBg]     = dImVec4(0.38f, 0.38f, 0.38f, 1.00f);
    colors[ImGuiCol_Tab]                = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabHovered]         = dImVec4(0.40f, 0.40f, 0.40f, 1.00f);
    colors[ImGuiCol_TabActive]          = dImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_TabUnfocused]       = dImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = dImVec4(0.33f, 0.33f, 0.33f, 1.00f);
    colors[ImGuiCol_DockingPreview]     = dImVec4(0.85f, 0.85f, 0.85f, 0.28f);

    if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    style.IndentSpacing     = 12;
}

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

static void setup_imgui_dockspace() {
    const ImGuiWindowFlags main_window_flags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_MenuBar;


    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("Main Window", nullptr, main_window_flags);
    ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);
    ImGui::End();

    ImGui::PopStyleVar(3);
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



// ---------------------------------------------- PLATFORM WINDOW ----------------------------------------------

ImGuiPlatform::PlatformWindow::PlatformWindow(ImGuiPlatform* parent, Window::Flags flags) :
        platform(parent),
        window({1280, 768}, "Window", flags),
        swapchain(&window),
        event_handler(std::make_unique<ImGuiEventHandler>(&window)) {

    window.set_event_handler(event_handler.get());
    window.show();
}

bool ImGuiPlatform::PlatformWindow::update_swapchain() {
    y_profile();

    if(swapchain.size() != window.size()) {
        wait_all_queues();
        swapchain.reset();
    }
    return swapchain.is_valid();
}

bool ImGuiPlatform::PlatformWindow::render(ImGuiViewport* viewport) {
    y_profile();

    if(!window.update()) {
        return false;
    }

    if(update_swapchain()) {
        const FrameToken token = swapchain.next_frame();
        CmdBufferRecorder recorder = create_disposable_cmd_buffer();

        {
            Framebuffer framebuffer(token.image_view);
            RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
            platform->_renderer->render(viewport->DrawData, pass);
        }

        swapchain.present(token, std::move(recorder), graphic_queue());
    }

    return true;
}


// ---------------------------------------------- PLATFORM ----------------------------------------------

ImGuiPlatform* ImGuiPlatform::_instance = nullptr;

ImGuiPlatform* ImGuiPlatform::instance() {
    y_debug_assert(_instance);
    return _instance;
}

ImGuiPlatform::ImGuiPlatform(bool multi_viewport) {
    y_always_assert(_instance == nullptr, "ImGuiPlatform instance already exists.");
    _instance = this;

    ImGui::CreateContext();

    setup_style();

    auto& io = ImGui::GetIO();

    y_always_assert(io.BackendPlatformUserData == nullptr, "ImGui already has a platform backend.");
    io.BackendPlatformUserData = this;
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    setup_key_bindings(io);
    setup_config_files(io);
    setup_backend_flags(io, multi_viewport);

    if(multi_viewport) {
        auto& platform = ImGui::GetPlatformIO();

        Y_TODO(do every frame)
        discover_monitors(platform);

        platform.Platform_CreateWindow          = [](ImGuiViewport* vp) {
            ImGuiPlatform* self = get_platform();
            vp->PlatformHandle = self->_windows.emplace_back(std::make_unique<PlatformWindow>(self, Window::NoDecoration)).get();
        };

        platform.Platform_DestroyWindow         = [](ImGuiViewport* vp) { get_platform()->close_window(get_platform_window(vp)); };
        platform.Platform_ShowWindow            = [](ImGuiViewport* vp) { get_window(vp)->show(); };
        platform.Platform_SetWindowPos          = [](ImGuiViewport* vp, ImVec2 pos) { get_window(vp)->set_position(pos); };
        platform.Platform_SetWindowSize         = [](ImGuiViewport* vp, ImVec2 size) { get_window(vp)->set_size(size); };
        platform.Platform_GetWindowPos          = [](ImGuiViewport* vp) { return ImVec2(get_window(vp)->position()); };
        platform.Platform_GetWindowSize         = [](ImGuiViewport* vp) { return ImVec2(get_window(vp)->size()); };

        platform.Platform_SetWindowTitle        = [](ImGuiViewport* vp, const char* title) { get_window(vp)->set_title(title); };

        platform.Platform_SetWindowFocus        = [](ImGuiViewport* vp) { get_window(vp)->focus(); };
        platform.Platform_GetWindowFocus        = [](ImGuiViewport* vp) { return get_window(vp)->has_focus(); };
        platform.Platform_GetWindowMinimized    = [](ImGuiViewport* vp) {return get_window(vp)->is_minimized(); };

        platform.Renderer_RenderWindow          = [](ImGuiViewport* vp, void*) { get_platform_window(vp)->render(vp); };

    }

    _renderer = std::make_unique<ImGuiRenderer>();
    _main_window = std::make_unique<PlatformWindow>(this, Window::Resizable);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    viewport->PlatformHandle = _main_window.get();
}

ImGuiPlatform::~ImGuiPlatform() {
    y_always_assert(_instance == this, "ImGuiPlatform instance has already been deleted.");
    _instance = nullptr;
}


const ImGuiRenderer* ImGuiPlatform::renderer() const {
    return _renderer.get();
}

Window* ImGuiPlatform::main_window() {
    return &_main_window->window;
}

bool ImGuiPlatform::exec(OnGuiFunc func, bool once) {
    while(_main_window->window.update()) {
        y_profile_zone("exec once");

        ImGui::GetIO().DeltaTime = std::max(math::epsilon<float>, float(_frame_timer.reset().to_secs()));
        ImGui::GetIO().DisplaySize = _main_window->window.size();

        if(_main_window->update_swapchain()) {
            const FrameToken frame = _main_window->swapchain.next_frame();
            CmdBufferRecorder recorder = create_disposable_cmd_buffer();

            {
                y_profile_zone("imgui");
                ImGui::NewFrame();

                setup_imgui_dockspace();

                if(_demo_window) {
                    ImGui::ShowDemoWindow(&_demo_window);
                }

                if(func) {
                    func(recorder);
                }

                ImGui::Render();
            }

            {
                y_profile_zone("main window");
                Framebuffer framebuffer(frame.image_view);
                RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
                _renderer->render(ImGui::GetDrawData(), pass);
            }

            {
                y_profile_zone("secondary windows");
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            _main_window->swapchain.present(frame, std::move(recorder), graphic_queue());
        }

        if(once) {
            return true;
        }
    }

    return false;
}


void ImGuiPlatform::close_window(PlatformWindow* window) {
    window->window.close();
    while(window->window.update()) {
        // nothing
    }

    for(auto it = _windows.begin(); it != _windows.end(); ++it) {
        if(it->get() == window) {
            _windows.erase_unordered(it);
            return;
        }
    }

    y_fatal("Window not found.");
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

void ImGuiPlatform::show_demo() {
    _demo_window = true;
}

}

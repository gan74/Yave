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

#ifndef EDITOR_IMGUIPLATFORM_H
#define EDITOR_IMGUIPLATFORM_H

#include "ImGuiRenderer.h"

#include <yave/window/Window.h>
#include <yave/window/EventHandler.h>
#include <yave/graphics/swapchain/Swapchain.h>

#include <y/core/Chrono.h>
#include <y/core/Vector.h>

#include <functional>

struct ImGuiViewport;

namespace editor {

ImGuiPlatform* imgui_platform();

class ImGuiPlatform : NonMovable {

    struct PlatformWindow : NonMovable {
        PlatformWindow(ImGuiPlatform* parent, Window::Flags flags);

        bool update_swapchain();

        bool render(ImGuiViewport* viewport);

        ImGuiPlatform* platform = nullptr;

        Window window;
        Swapchain swapchain;
        std::unique_ptr<EventHandler> event_handler;
    };

    public:
        using OnGuiFunc = std::function<void(CmdBufferRecorder&)>;

        ImGuiPlatform(bool multi_viewport = true);
        ~ImGuiPlatform();

        static ImGuiPlatform* instance();

        const ImGuiRenderer* renderer() const;

        Window* main_window();

        bool exec(OnGuiFunc func = nullptr, bool once = false);

        void show_demo();

    private:
        friend struct PlatformWindow;

        void close_window(PlatformWindow* window);

        static ImGuiPlatform* get_platform();
        static Window* get_window(ImGuiViewport* vp);
        static PlatformWindow* get_platform_window(ImGuiViewport* vp);

    private:
        static ImGuiPlatform* _instance;

        std::unique_ptr<PlatformWindow> _main_window;
        core::Vector<std::unique_ptr<PlatformWindow>> _windows;

        std::unique_ptr<ImGuiRenderer> _renderer;

        core::Chrono _frame_timer;

        bool _demo_window = is_debug_defined;
};

}

#endif // EDITOR_IMGUIPLATFORM_H

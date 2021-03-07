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

#include <editor/editor.h>
#include <editor/ImGuiRenderer2.h>

#include <yave/graphics/swapchain/Swapchain.h>
#include <yave/window/Window.h>

#include <y/core/Chrono.h>
#include <y/core/Vector.h>

struct ImGuiViewport;

namespace editor {

class ImGuiPlatform : NonMovable {

    struct PlatformWindow : NonMovable {
        PlatformWindow(ImGuiPlatform* parent, Window::Flags flags = Window::NoDecoration);

        void render(ImGuiViewport* viewport);

        ImGuiPlatform* platform = nullptr;
        Window window;
        Swapchain swapchain;
    };

    public:
        ImGuiPlatform(DevicePtr dptr, bool multi_viewport = true);

        DevicePtr device() const;

        void run();

    private:
        static ImGuiPlatform* get_platform();
        static Window* get_window(ImGuiViewport* vp);
        static PlatformWindow* get_platform_window(ImGuiViewport* vp);

        friend class PlatformWindow;

    private:
        std::unique_ptr<PlatformWindow> _main_window;
        core::Vector<std::unique_ptr<PlatformWindow>> _windows;

        std::unique_ptr<ImGuiRenderer2> _renderer;

        core::Chrono _frame_timer;
};

}

#endif // EDITOR_IMGUIPLATFORM_H

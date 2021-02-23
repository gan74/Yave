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

#include "MainWindow.h"

#include <editor/context/EditorContext.h>

#include <yave/graphics/swapchain/Swapchain.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/Queue.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

MainWindow::MainWindow(ContextPtr cptr) :
        Window({1280, 768}, "Yave", Window::Flags(Window::Resizable/* | Window::HideConsole*/)),
        ContextLinked(cptr) {
}

MainWindow::~MainWindow() {
}

Swapchain* MainWindow::swapchain() {
    return _swapchain.get();
}

void MainWindow::resized() {
    create_swapchain();
}

void MainWindow::create_swapchain() {
    y_profile();
    // needed because the swapchain immediatly destroys it images
    wait_all_queues(device());

    if(_swapchain) {
        _swapchain->reset();
    } else {
        _swapchain = std::make_unique<Swapchain>(device(), static_cast<Window*>(this));
    }
}

void MainWindow::present(CmdBufferRecorder& recorder, const FrameToken& token) {
    y_profile();

    _swapchain->present(token, recorder, graphic_queue(device()));
}

}


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

#include <editor/utils/crashhandler.h>
#include <editor/events/MainEventHandler.h>
#include <editor/context/EditorContext.h>

#include <yave/graphics/device/Device.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/swapchain/Swapchain.h>
#include <yave/assets/SQLiteAssetStore.h>

#include <y/core/Chrono.h>
#include <y/concurrent/concurrent.h>

#include <y/utils/log.h>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

using namespace editor;

static EditorContext* context = nullptr;

#ifdef Y_DEBUG
static bool display_console = true;
static bool debug_instance = true;
#else
static bool display_console = false;
static bool debug_instance = false;
#endif



static void hide_console() {
#ifdef Y_OS_WIN
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
}

static void parse_args(int argc, char** argv) {
    for(std::string_view arg : core::Span<const char*>(argv + 1, argc - 1)) {
        if(arg == "--nodebug") {
            debug_instance = false;
        } else if(arg == "--debug") {
            debug_instance = true;
        } else if(arg == "--console") {
            display_console = true;
        }
#ifdef Y_DEBUG
        else if(arg == "--errbreak") {
            core::result::break_on_error = true;
        }
#endif
        else {
            log_msg(fmt("Unknown argumeent: %", arg), Log::Error);
        }
    }

    if(!display_console) {
        hide_console();
    }
    y_debug_assert([] { log_msg("Debug asserts enabled."); return true; }());
}

static Instance create_instance() {
    y_profile();
    if(!debug_instance) {
        log_msg("Vulkan debugging disabled.", Log::Warning);
    }
    return Instance(debug_instance ? DebugParams::debug() : DebugParams::none());
}

static Device create_device(Instance& instance) {
    y_profile();
    return Device(instance);
}

static EditorContext create_context(const Device& device) {
    y_profile();
    return EditorContext(&device);
}

int main(int argc, char** argv) {
    concurrent::set_thread_name("Main thread");

    parse_args(argc, argv);

    if(!crashhandler::setup_handler()) {
        log_msg("Unable to setup crash handler.", Log::Warning);
    }

    Instance instance = create_instance();


    Device device = create_device(instance);
    EditorContext ctx = create_context(device);
    context = &ctx;

    MainWindow window(&ctx);
    window.set_event_handler(std::make_unique<MainEventHandler>());
    window.show();

    for(;;) {
        if(!window.update()) {
            if(ctx.ui_manager().confirm("Quit ?")) {
                break;
            } else {
                window.show();
            }
        }

        // 35 ms to not spam if we are capped at 30 FPS
        core::DebugTimer frame_timer("frame", core::Duration::milliseconds(35.0));

        ctx.world().tick();

        Swapchain* swapchain = window.swapchain();
        if(swapchain && swapchain->is_valid()) {
            FrameToken frame = swapchain->next_frame();
            CmdBufferRecorder recorder(device.create_disposable_cmd_buffer());

            ctx.ui_manager().paint(recorder, frame);

            window.present(recorder, frame);
        }

        ctx.flush_deferred();

        if(ctx.device_resources_reload_requested()) {
            device.device_resources().reload();
            ctx.resources().reload();
            ctx.set_device_resource_reloaded();
        }
    }

    context = nullptr;

    return 0;
}


/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <csignal>

#include <editor/events/MainEventHandler.h>
#include <editor/context/EditorContext.h>

#include <yave/graphics/swapchain/Swapchain.h>

using namespace editor;


EditorContext* context = nullptr;
bool debug_instance = false;
bool display_console = false;



static void hide_console() {
#ifdef Y_OS_WIN
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
}

static void crash_handler(int) {
	log_msg("SEGFAULT!");
	Y_TODO(we might want to save whatever we can here)
}

static void setup_handlers() {
	std::signal(SIGSEGV, crash_handler);
	y_debug_assert([] { log_msg("Debug assert enabled"); return true; }());
	perf::set_output_file("perfdump.json");
}

static void parse_args(int argc, char** argv) {
	for(std::string_view arg : core::ArrayView<const char*>(argv, argc)) {
		if(arg == "--nodebug") {
			debug_instance = false;
		}
		if(arg == "--console") {
			display_console = true;
		}
	}

	if(!display_console) {
		hide_console();
	}
}

static void setup_logger() {
	set_log_callback([](std::string_view msg, Log type, void*) {
			if(context) {
				context->log_message(msg, type);
			}
			return !display_console;
		});
}

static Instance create_instance() {
	if(!debug_instance) {
		log_msg("Vulkan debugging disabled", Log::Warning);
	}
	return Instance(debug_instance ? DebugParams::debug() : DebugParams::none());
}





int main(int argc, char** argv) {
	parse_args(argc, argv);
	setup_handlers();
	setup_logger();

	Instance instance = create_instance();

	Device device(instance);
	EditorContext ctx(&device);
	context = &ctx;

	MainWindow window(&ctx);
	window.set_event_handler(std::make_unique<MainEventHandler>());
	window.show();

	for(;;) {
		if(!window.update()) {
			if(ctx.ui().confirm("Quit ?")) {
				break;
			} else {
				window.show();
			}
		}

		Swapchain* swapchain = window.swapchain();
		if(swapchain && swapchain->is_valid()) {
			FrameToken frame = swapchain->next_frame();
			CmdBufferRecorder recorder(device.create_disposable_cmd_buffer());

			ctx.ui().paint(recorder, frame);

			window.present(recorder, frame);
		}

		ctx.flush_deferred();
	}

	return 0;
}


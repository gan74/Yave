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



static void crash_handler(int) {
	log_msg("SEGFAULT!");
	Y_TODO(we might want to save whatever we can here)
}

static void setup_handlers() {
	std::signal(SIGSEGV, crash_handler);
	y_debug_assert([] { log_msg("Debug assert enabled"); return true; }());
	perf::set_output_file("perfdump.json");
}


static Instance create_instance(int argc, char** argv) {
	bool debug = true;
	for(std::string_view arg : core::ArrayView<const char*>(argv, argc)) {
		if(arg == "--nodebug") {
			log_msg("Vulkan debugging disabled", Log::Warning);
			debug = false;
		}
	}
	return Instance(debug ? DebugParams::debug() : DebugParams::none());
}


int main(int argc, char** argv) {

	setup_handlers();
	Instance instance = create_instance(argc, argv);

	Device device(instance);
	EditorContext ctx(&device);
	context = &ctx;

	MainWindow window(&ctx);
	window.set_event_handler(std::make_unique<MainEventHandler>());
	window.show();

	for(;;) {
		if(!window.update() && ctx.ui().confirm("Quit ?")) {
			break;
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


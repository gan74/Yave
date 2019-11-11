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

#include <yave/assets/SQLiteAssetStore.h>
#include <y/core/Chrono.h>

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

[[maybe_unused]]
static void crash_handler(int) {
	log_msg("SEGFAULT!");
	Y_TODO(we might want to save whatever we can here)
}

static void setup_handlers() {
	std::signal(SIGSEGV, crash_handler);
	perf::set_output_file("perfdump.json");
}

static void parse_args(int argc, char** argv) {
	for(std::string_view arg : core::Span<const char*>(argv, argc)) {
		if(arg == "--nodebug") {
			debug_instance = false;
		}
		if(arg == "--debug") {
			debug_instance = true;
		}
		if(arg == "--console") {
			display_console = true;
		}
	}

	if(!display_console) {
		hide_console();
	}
	y_debug_assert([] { log_msg("Debug asserts enabled."); return true; }());
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

static EditorContext create_constext(const Device& device) {
	y_profile();
	return EditorContext(&device);
}



int main(int argc, char** argv) {
	setup_handlers();
	FileSystemModel::local_filesystem()->remove("./test.sqlite3").expected("Unable to delete database");
	SQLiteAssetStore store("test.sqlite3");
	const FileSystemModel* fs = store.filesystem();
	core::Chrono timer;
	{
		{
			auto r = fs->exists("doesntexists");
			y_debug_assert(r.unwrap() == false);
		}
		{
			log_msg(fmt("exits = %", fs->exists("maybeexsits").unwrap()));
			auto r = fs->create_directory("maybeexsits");
			log_msg(fmt("exited = %", r.is_error()));
			fs->remove("maybeexsits").unwrap();
			log_msg("deleted");
		}
		{
			fs->create_directory("temp").unwrap();
			fs->rename("temp", "temp2").unwrap();
			fs->remove("temp2").unwrap();
			y_debug_assert(fs->exists("temp2").unwrap() == false);
			y_debug_assert(fs->exists("temp").unwrap() == false);
		}
		{
			y_debug_assert(fs->filename("pwet/foo") == "foo");
			y_debug_assert(fs->filename("pwet/") == "");
			y_debug_assert(fs->filename("pwet") == "pwet");
			y_debug_assert(fs->parent_path("pwet/foo").unwrap() == "pwet");
		}
	}
	log_msg(fmt("Ok (%ms)", timer.reset().to_millis()));


	{
		const u64 data = 0xabcdef0123456789;
		io2::Buffer buffer;
		buffer.write_one(data).unwrap();
		buffer.reset();

		AssetId id = store.import(buffer, "folder/file").unwrap();
		y_debug_assert(id != AssetId::invalid_id());
		y_debug_assert(fs->is_directory("folder").unwrap());
		y_debug_assert(fs->exists("folder/file").unwrap());

		{
			io2::ReaderPtr reader = std::move(store.data(id).unwrap());
			u64 d = 0;
			reader->read_one(d).unwrap();
			y_debug_assert(d == data);
			y_debug_assert(reader->at_end());
		}

		fs->remove("folder/file").unwrap();
		y_debug_assert(!fs->exists("folder/file").unwrap());
		y_debug_assert(fs->exists("folder/").unwrap());
		y_debug_assert(store.id("folder/file").is_error());

	}

	return 0;



	parse_args(argc, argv);
	setup_handlers();
	setup_logger();

	Instance instance = create_instance();


	Device device = create_device(instance);
	EditorContext ctx = create_constext(device);
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




	set_log_callback(nullptr);
	context = nullptr;


	return 0;
}


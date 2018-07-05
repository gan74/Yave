
#include "MainWindow.h"

#include <y/io/File.h>
#include <yave/serialize/filesystem.h>

using namespace editor;

int main(int argc, char** argv) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));

	bool debug = true;
	for(std::string_view arg : core::ArrayView<const char*>(argv, argc)) {
		if(arg == "--nodebug") {
			log_msg("Vulkan debugging disabled", Log::Warning);
			debug = false;
		}
	}

	Instance instance(debug ? DebugParams::debug() : DebugParams::none());
	Device device(instance);

	EditorContext ctx(&device);

	MainWindow window(&ctx);
	window.exec();

	return 0;
}

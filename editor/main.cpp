
#include "MainWindow.h"

#include <y/io/File.h>

using namespace editor;

int main(int argc, char** argv) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));

	bool debug = true;
	for(core::String arg : core::ArrayView<const char*>(argv, argc)) {
		if(arg == "--nodebug") {
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

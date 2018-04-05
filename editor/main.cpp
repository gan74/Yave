
#include "MainWindow.h"

#include <y/io/File.h>

using namespace editor;

int main(int, char **) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));

	//AssetLoader<core::String> loader;

	Instance instance(DebugParams::debug());
	Device device(instance);

	EditorContext ctx(&device);

	MainWindow window(&ctx);
	window.exec();

	return 0;
}

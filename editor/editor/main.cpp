
#include "MainWindow.h"

#include <y/io/File.h>

using namespace editor;

int main(int, char **) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));

	MainWindow window(DebugParams::debug());
	window.exec();

	return 0;
}

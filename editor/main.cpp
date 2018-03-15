
#include "App.h"

#include <y/io/File.h>
#include <EventHandler.h>

using namespace editor;
using namespace core;

int main(int, char **) {
	log_msg("starting...");

	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));

	ImGui::CreateContext();

	Window win({1280, 768}, "Yave");
	win.set_event_handler(new editor::EventHandler());

	App app(DebugParams::debug());
	app.set_swapchain(&win);

	win.show();
	while(win.update()) {
		app.update(math::Vec2());
		app.draw();
	}

	return 0;
}

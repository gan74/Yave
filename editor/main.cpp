
#include "MainWindow.h"
#include <y/io/File.h>
#include <yave/framegraph/renderers.h>

#include <editor/utils/renderdochelper.h>

using namespace editor;


int test_graph() {
	Instance instance(DebugParams::debug());
	Device device(instance);
	Scene scene;

	{
		auto cap = renderdoc::start_capture();

		auto resources = std::make_shared<FrameGraphResourcePool>(&device);
		SceneView scene_view(scene);

		for(usize i = 0; i != 5; ++i) {
			FrameGraph graph(resources);

			auto gbuffer = render_gbuffer(graph, &scene_view, math::Vec2ui(512));

			{
				CmdBufferRecorder rec(device.create_disposable_cmd_buffer());
				std::move(graph).render(rec);
				RecordedCmdBuffer cmd(std::move(rec));
				device.queue(vk::QueueFlagBits::eGraphics).submit<SyncSubmit>(std::move(cmd));
			}

			log_msg(fmt("allocated resource count = %", resources->allocated_resources()));
		}
	}

	return 4;
}

int main(int argc, char** argv) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));

	/*test_graph();
	return 0;*/

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

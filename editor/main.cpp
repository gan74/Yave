
#include "MainWindow.h"

#include <y/io/File.h>

#include <yave/framegraph/FrameGraph.h>

using namespace editor;


int test_graph() {
	Instance instance(DebugParams::debug());
	Device device(instance);

	{
		FrameGraph graph(&device);

		FrameGraphResource<Texture> b_output;
		FrameGraphResource<Texture> a_output;
		struct Pass {
			FrameGraphResource<StorageTexture> storage;
			FrameGraphResource<Texture> texture;
		};

		auto a_pass = graph.add_callback_pass<Pass>("A pass",
			[&](FrameGraphBuilder& builder, Pass& pass) {
				unused(builder, pass);
				builder.create(pass.storage);
				builder.write(pass.storage);
				builder.create(a_output);
			},
			[](CmdBufferRecorder& recorder, const Pass& pass, const FrameGraphResources& res) {
				unused(recorder, pass, res);
			});

		graph.add_callback_pass<Pass>("B pass",
			[&](FrameGraphBuilder& builder, Pass& pass) {
				unused(builder, pass);
				pass.texture = builder.create<Texture>();
				builder.read(a_pass.storage);
				builder.write(b_output);
			},
			[&](CmdBufferRecorder& recorder, const Pass& pass, const FrameGraphResources& res) {
				unused(recorder, pass, res);
				res.get<StorageTexture>(a_pass.storage);
				res.get<Texture>(pass.texture);
			});

		{
			CmdBufferRecorder rec(device.create_disposable_cmd_buffer());
			graph.render(rec, b_output);
			RecordedCmdBuffer cmd(std::move(rec));
			device.queue(vk::QueueFlagBits::eGraphics).submit<SyncSubmit>(std::move(cmd));
		}
	}

	return 4;
}

int main(int argc, char** argv) {
	perf::set_output(std::move(io::File::create("perfdump.json").unwrap()));


	/*log_msg("Testing frame graph");

	return test_graph();*/




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


#include "MainWindow.h"

#include <y/io/File.h>

#include <yave/rendergraph/RenderGraph.h>

using namespace editor;


int test_graph() {
	Instance instance(DebugParams::debug());
	Device device(instance);

	{
		RenderGraph graph(&device);

		RenderGraphResource<Texture> b_output;
		RenderGraphResource<Texture> a_output;
		struct Pass {
				RenderGraphResource<StorageTexture> storage;
				RenderGraphResource<Texture> texture;
		};

		graph.add_callback_pass<Pass>(
			[&](RenderGraphBuilder& builder, Pass& pass) {
				unused(builder, pass);
				builder.create(pass.storage);
				builder.write(pass.storage);
				builder.create(a_output);
			},
			[](CmdBufferRecorderBase& recorder, const Pass& pass, const RenderGraphResources& res) {
				unused(recorder, pass, res);
				log_msg("A");
			});

		graph.add_callback_pass<Pass>(
			[&](RenderGraphBuilder& builder, Pass& pass) {
				unused(builder, pass);
				b_output = builder.create<Texture>();
				builder.write(b_output);
			},
			[](CmdBufferRecorderBase& recorder, const Pass& pass, const RenderGraphResources& res) {
				unused(recorder, pass, res);
				log_msg("B");
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


	/*log_msg("Testing render graph");

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

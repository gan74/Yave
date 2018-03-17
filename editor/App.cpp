/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include "App.h"

#include <yave/images/ImageData.h>
#include <yave/buffers/TypedBuffer.h>
#include <yave/renderers/RenderingPipeline.h>
#include <yave/renderers/ToneMapper.h>
#include <yave/commands/TimeQuery.h>

#include "scenes.h"

namespace editor {

static core::Chrono time;

App::App(DebugParams params) : instance(params), device(instance), thread_device(device.thread_data()) {
	{ Y_LOG_PERF(""); }
	log_msg("sizeof(core::Vector) = "_s + sizeof(core::Vector<int>));
	log_msg("sizeof(StaticMesh) = "_s + sizeof(StaticMeshInstance));
	log_msg("sizeof(Matrix4) = "_s + sizeof(math::Matrix4<>));
	log_msg("sizeof(DeviceMemory) = "_s + sizeof(DeviceMemory));
	log_msg("sizeof(Texture) = "_s + sizeof(Texture));
	log_msg("sizeof(TextureView) = "_s + sizeof(TextureView));

	create_assets();
}

App::~App() {
	device.queue(QueueFamily::Graphics).wait();

	scene_view = nullptr;
	scene = nullptr;

	swapchain = nullptr;
	renderer = nullptr;
}

void App::draw() {
	Y_LOG_PERF("draw,rendering");

	{
		Y_LOG_PERF("draw,rendering,gui");
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize.x = swapchain->size().x();
		io.DisplaySize.y = swapchain->size().y();

		ImGui::NewFrame();
		ImGui::ShowDemoWindow();
		if (ImGui::Checkbox("Cap framerate", &cap_framerate))
		{
			// do stuff
		}
		ImGui::EndFrame();
		ImGui::Render();
	}




	FrameToken frame = swapchain->next_frame();
	CmdBufferRecorder<> recorder(device.create_cmd_buffer());
	/*TimeQuery timer(&device);
	timer.start(recorder);*/

	{
		/*RenderingPipeline pipeline(frame);
		pipeline.dispatch(renderer, recorder);*/

		RenderingPipeline pipeline(renderer);
		pipeline.render(recorder, frame);
	}

	RecordedCmdBuffer<> cmd_buffer(std::move(recorder));

	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = device.queue(QueueFamily::Graphics).vk_queue();
	auto vk_buffer = cmd_buffer.vk_cmd_buffer();

	graphic_queue.submit(vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&frame.image_aquired)
			.setPWaitDstStageMask(&pipe_stage_flags)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&vk_buffer)
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&frame.render_finished),
		cmd_buffer.vk_fence());

	swapchain->present(frame, graphic_queue);

	perf::event("rendering", "frame present");


	if(cap_framerate) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void App::update(math::Vec2 angles) {
	float dist = 200.0f;

	auto& camera = scene_view->camera();

	auto cam_tr = math::rotation({0, 0, -1}, angles.x()) * math::rotation({0, 1, 0}, angles.y());
	auto cam_pos = cam_tr * math::Vec4(dist, 0, 0, 1);
	auto cam_up = cam_tr * math::Vec4(0, 0, 1, 0);

	camera.set_view(math::look_at(cam_pos.to<3>() / cam_pos.w(), math::Vec3(), cam_up.to<3>()));
	camera.set_proj(math::perspective(math::to_rad(60.0f), 4.0f / 3.0f, 1.0f));

	/*log_msg("forward = (" +
			core::str(camera.forward().x()) + ", " +
			core::str(camera.forward().y()) + ", " +
			core::str(camera.forward().z()) + ")");*/
}

void App::create_assets() {
	auto [sce, ve] = create_scene(&device);
	scene = std::move(sce);
	scene_view = std::move(ve);

	update();
}


void App::create_renderers() {
	Y_LOG_PERF("init,loading");

	using namespace experimental;

	/*auto gui = core::Arc<SecondaryRenderer>(new ImGuiRenderer(&device));
	renderer = new ScreenEndOfPipe(gui);*/

	auto scene = core::Arc<SceneRenderer>(new SceneRenderer(&device, *scene_view));
	auto gbuffer = core::Arc<GBufferRenderer>(new GBufferRenderer(scene, swapchain->size()));
	auto deferred = core::Arc<TiledDeferredRenderer>(new TiledDeferredRenderer(gbuffer));
	auto tonemap = core::Arc<ToneMapper>(new ToneMapper(deferred));
	renderer = new SimpleEndOfPipe({tonemap});
}

}


/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#include "MainEventHandler.h"

#include <renderers/ImGuiRenderer.h>

#include <yave/renderers/SimpleEndOfPipe.h>
#include <yave/renderers/ToneMapper.h>
#include <yave/renderers/GBufferRenderer.h>
#include <yave/renderers/TiledDeferredRenderer.h>

#include <widgets/PerformanceWidget.h>

#include "scenes.h"

namespace editor {

MainWindow::MainWindow(DebugParams params) : Window({1280, 768}, "Yave"), _instance(params), _device(_instance) {

	ImGui::CreateContext();

	_widgets << new PerformanceWidget();

	create_swapchain();
	create_renderer();

	set_event_handler(new MainEventHandler());
}

void MainWindow::create_renderer() {
	auto [sce, ve] = create_scene(&_device);
	_scene = std::move(sce);
	_scene_view = std::move(ve);


	auto scene		= Node::Ptr<SceneRenderer>(new SceneRenderer(&_device, *_scene_view));
	auto gbuffer	= Node::Ptr<GBufferRenderer>(new GBufferRenderer(scene, _swapchain->size()));
	auto deferred	= Node::Ptr<TiledDeferredRenderer>(new TiledDeferredRenderer(gbuffer));
	auto tonemap	= Node::Ptr<ToneMapper>(new ToneMapper(deferred));

	auto gui		= Node::Ptr<SecondaryRenderer>(new ImGuiRenderer(&_device));
	_renderer = new SimpleEndOfPipe({tonemap, gui});
}

void MainWindow::exec() {
	show();

	while(update()) {
		draw_ui();
		render();
	}

	_device.queue(QueueFamily::Graphics).wait();
}

void MainWindow::draw_ui() {
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = size().x();
	io.DisplaySize.y = size().y();


	ImGui::NewFrame();

	for(auto& w : _widgets) {
		w->paint();
	}

	ImGui::EndFrame();
	ImGui::Render();
}

void MainWindow::create_swapchain() {
	_device.queue(QueueFamily::Graphics).wait();
	_renderer = nullptr;

	if(_swapchain) {
		_swapchain->reset();
	} else {
		_swapchain = new Swapchain(&_device, this);
	}
}

void MainWindow::render() {
	FrameToken frame = _swapchain->next_frame();

	CmdBufferRecorder<> recorder(_device.create_cmd_buffer());
	{
		RenderingPipeline pipeline(_renderer);
		pipeline.render(recorder, frame);
	}

	RecordedCmdBuffer<> cmd_buffer(std::move(recorder));

	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = _device.queue(QueueFamily::Graphics).vk_queue();
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

	_swapchain->present(frame, graphic_queue);
}

}

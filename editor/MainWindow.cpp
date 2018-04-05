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

#include <editor/events/MainEventHandler.h>
#include <editor/renderers/ImGuiRenderer.h>

#include <yave/renderers/SimpleEndOfPipe.h>
#include <yave/renderers/ToneMapper.h>
#include <yave/renderers/GBufferRenderer.h>
#include <yave/renderers/TiledDeferredRenderer.h>
#include <yave/renderers/FramebufferRenderer.h>
#include <y/core/SmallVector.h>

#include <editor/views/EntityView.h>
#include <editor/views/EngineView.h>
#include <editor/views/AssetBrowser.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/PerformanceMetrics.h>

#include <imgui/imgui.h>

namespace editor {

MainWindow::MainWindow(ContextPtr cptr) :
		Window({1280, 768}, "Yave", Window::Resizable),
		ContextLinked(cptr) {

	ImGui::CreateContext();
	ImGui::InitDock();
	ImGui::GetIO().IniFilename = "editor.ini";
	ImGui::GetIO().LogFilename = "editor_logs.txt";

	_elements << std::make_unique<EngineView>(context());
	_elements << std::make_unique<EntityView>(context());

	auto gui = Node::Ptr<SecondaryRenderer>(new ImGuiRenderer(device()));
	_ui_renderer = new SimpleEndOfPipe(gui);

	set_event_handler(new MainEventHandler());
}

MainWindow::~MainWindow() {
	ImGui::DestroyContext();
}


void MainWindow::resized() {
	create_swapchain();
}

void MainWindow::create_swapchain() {
	// needed because the swapchain imediatly destroys it images
	device()->queue(vk::QueueFlagBits::eGraphics).wait();

	if(_swapchain) {
		_swapchain->reset();
	} else {
		_swapchain = std::make_unique<Swapchain>(device(), static_cast<Window*>(this));
	}
}

void MainWindow::exec() {
	show();

	while(update()) {

		FrameToken frame = _swapchain->next_frame();
		CmdBufferRecorder<> recorder(device()->create_cmd_buffer());

		render(recorder, frame);
		present(recorder, frame);
	}

	device()->queue(QueueFamily::Graphics).wait();
}

void MainWindow::present(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	RecordedCmdBuffer<> cmd_buffer(std::move(recorder));

	vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
	auto graphic_queue = device()->queue(QueueFamily::Graphics).vk_queue();
	auto vk_buffer = cmd_buffer.vk_cmd_buffer();

	graphic_queue.submit(vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&token.image_aquired)
			.setPWaitDstStageMask(&pipe_stage_flags)
			.setCommandBufferCount(1)
			.setPCommandBuffers(&vk_buffer)
			.setSignalSemaphoreCount(1)
			.setPSignalSemaphores(&token.render_finished),
		cmd_buffer.vk_fence());

	_swapchain->present(token, graphic_queue);
}


template<typename T>
static void show_element(ContextPtr cptr, core::Vector<std::unique_ptr<UiElement>>& elems) {
	for(auto& e : elems) {
		if(auto t = dynamic_cast<T*>(e.get())) {
			t->show();
			return;
		}
	}
	if constexpr(std::is_constructible_v<T, ContextPtr>) {
		elems << std::make_unique<T>(cptr);
	} else {
		unused(cptr);
		elems << std::make_unique<T>();
	}
}

void MainWindow::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	ImU32 flags = ImGuiWindowFlags_NoTitleBar |
				  ImGuiWindowFlags_NoResize |
				  ImGuiWindowFlags_NoScrollbar |
				  ImGuiWindowFlags_NoSavedSettings |
				  ImGuiWindowFlags_NoBringToFrontOnFocus |
				  ImGuiWindowFlags_MenuBar;

	ImGui::GetIO().DisplaySize = math::Vec2(_swapchain->size());

	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::Begin("Main window", nullptr, flags);

	// menu
	{
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("View")) {
				if(ImGui::MenuItem("Engine view"))		show_element<EngineView>(context(), _elements);
				if(ImGui::MenuItem("Entities"))			show_element<EntityView>(context(), _elements);
				if(ImGui::MenuItem("Asset browser"))	show_element<AssetBrowser>(context(), _elements);
				if(ImGui::MenuItem("Settings"))			show_element<SettingsPanel>(context(), _elements);


				if(ImGui::BeginMenu("Debug")) {
					if(ImGui::MenuItem("Camera debug")) show_element<CameraDebug>(context(), _elements);
					if(ImGui::MenuItem("Performances")) show_element<PerformanceMetrics>(context(), _elements);
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}


	// main UI
	{
		ImGui::BeginDockspace();
		{
			for(auto& e : _elements) {
				e->paint(recorder, token);
			}
		}
		ImGui::EndDockspace();
	}


	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();



	// render ui pipeline into cmd buffer
	{
		RenderingPipeline pipeline(_ui_renderer);
		pipeline.render(recorder, token);
	}
}


}

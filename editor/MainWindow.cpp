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
#include <editor/context/EditorContext.h>

#include <yave/renderers/SimpleEndOfPipe.h>
#include <yave/renderers/ToneMapper.h>
#include <yave/renderers/GBufferRenderer.h>
#include <yave/renderers/TiledDeferredRenderer.h>
#include <yave/renderers/FramebufferRenderer.h>
#include <y/core/SmallVector.h>

#include <editor/widgets/EntityView.h>
#include <editor/widgets/FileBrowser.h>
#include <editor/widgets/PropertyPanel.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/MemoryInfo.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/SceneDebug.h>

#include <editor/EngineView.h>

#include <imgui/imgui.h>

namespace editor {

MainWindow::MainWindow(ContextPtr cptr) :
		Window({1280, 768}, "Yave", Window::Resizable),
		ContextLinked(cptr) {

	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = "editor.ini";
	ImGui::GetIO().LogFilename = "editor_logs.txt";

	Node::Ptr<SecondaryRenderer> gui(new ImGuiRenderer(device()));
	_ui_renderer = std::make_shared<SimpleEndOfPipe>(gui);

	set_event_handler(new MainEventHandler());

	_engine_view = std::make_unique<EngineView>(context());
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
	do {
		show();
		while(update()) {
			if(_swapchain->size().x() && _swapchain->size().y()) {
				FrameToken frame = _swapchain->next_frame();
				CmdBufferRecorder<> recorder(device()->create_cmd_buffer());

				render(recorder, frame);
				present(recorder, frame);
			}
		}
	} while(!context()->ui().confirm("Quit ?"));

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

	context()->flush_deferred();
}

void MainWindow::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	ImU32 flags = ImGuiWindowFlags_NoTitleBar |
				  ImGuiWindowFlags_NoResize |
				  ImGuiWindowFlags_NoMove |
				  ImGuiWindowFlags_NoScrollbar |
				  ImGuiWindowFlags_NoSavedSettings |
				  ImGuiWindowFlags_NoBringToFrontOnFocus |
				  ImGuiWindowFlags_MenuBar;


	ImGui::GetIO().DisplaySize = math::Vec2(_swapchain->size());

	ImGui::NewFrame();
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	ImGui::Begin("Main window", nullptr, flags);

	render_ui(recorder, token);

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();


	// render ui pipeline into cmd buffer
	{
		RenderingPipeline pipeline(_ui_renderer);
		pipeline.render(recorder, token);
	}
}

void MainWindow::render_ui(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	// demo
	ImGui::ShowDemoWindow();


	// menu
	{
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("File")) {
				if(ImGui::MenuItem("Import")) context()->ui().show<AssetImporter>();
				ImGui::EndMenu();
			}


			if(ImGui::BeginMenu("View")) {
				if(ImGui::MenuItem("Entity view")) context()->ui().show<EntityView>();

				if(ImGui::BeginMenu("Debug")) {
					if(ImGui::MenuItem("Camera debug")) context()->ui().show<CameraDebug>();
					if(ImGui::MenuItem("Scene debug")) context()->ui().show<SceneDebug>();

					y_debug_assert(!ImGui::MenuItem("Debug assert"));

					ImGui::EndMenu();
				}
				if(ImGui::BeginMenu("Statistics")) {
					if(ImGui::MenuItem("Performances")) context()->ui().show<PerformanceMetrics>();
					if(ImGui::MenuItem("Memory info")) context()->ui().show<MemoryInfo>();
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if(ImGui::MenuItem("Settings")) context()->ui().show<SettingsPanel>();

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}


	// toolbar
	{
		float toolbar_size = 24.0f;
		ImVec2 button_size(toolbar_size, toolbar_size);

		if(ImGui::ImageButton(&context()->icons().save(), button_size)) {
			auto browser = context()->ui().show<FileBrowser>();
			browser->set_callback(
					[=](core::String filename) { context()->scene().save(filename); }
				);
		}

		ImGui::SameLine(0.0f, 0.1f);

		if(ImGui::ImageButton(&context()->icons().load(), button_size)) {
			auto browser = context()->ui().show<FileBrowser>();
			browser->set_callback(
					[=](core::String filename) { context()->scene().load(filename); }
				);
		}
	}

	if(context()->selection().selected()) {
		context()->ui().show<PropertyPanel>();
	}

	//ImGui::NextColumn();

	// engine view and overlay
	{
		ImGui::BeginChild("Engine view");

		_engine_view->paint(recorder, token);

		// main UI
		context()->ui().paint(recorder, token);

		ImGui::EndChild();
	}

}

}

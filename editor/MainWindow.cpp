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

#include <editor/views/EntityView.h>
#include <editor/views/EngineView.h>
#include <editor/views/FileBrowser.h>
#include <editor/views/AssetBrowser.h>
#include <editor/views/PropertyPanel.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/MemoryInfo.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/SceneDebug.h>

#include <imgui/imgui.h>

namespace editor {

bool confirm(const char* message) {
#ifdef Y_OS_WIN
	return MessageBox(GetActiveWindow(), message, "Confirm", MB_OKCANCEL) != IDCANCEL;
#else
	return true;
#endif
}

template<typename T>
static T* find_element(const core::Vector<std::unique_ptr<UiElement>>& elems) {
	for(auto& e : elems) {
		if(auto t = dynamic_cast<T*>(e.get())) {
			return t;
		}
	}
	return nullptr;
}

template<typename T>
static T* show_element(ContextPtr cptr, core::Vector<std::unique_ptr<UiElement>>& elems) {
	if(auto t = find_element<T>(elems)) {
		t->show();
		return t;
	}
	if constexpr(std::is_constructible_v<T, ContextPtr>) {
		elems << std::make_unique<T>(cptr);
	} else {
		unused(cptr);
		elems << std::make_unique<T>();
	}
	return dynamic_cast<T*>(elems.last().get());
}


MainWindow::MainWindow(ContextPtr cptr) :
		Window({1280, 768}, "Yave", Window::Resizable),
		ContextLinked(cptr) {

	ImGui::CreateContext();
	ImGui::InitDock();
	ImGui::GetIO().IniFilename = "editor.ini";
	ImGui::GetIO().LogFilename = "editor_logs.txt";

	Node::Ptr<SecondaryRenderer> gui(new ImGuiRenderer(device()));
	_ui_renderer = std::make_shared<SimpleEndOfPipe>(gui);

	set_event_handler(new MainEventHandler());

	{
		_elements << std::make_unique<EngineView>(context());
		_elements << std::make_unique<EntityView>(context());
		_elements << std::make_unique<PropertyPanel>(context());
	}

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
	} while(!confirm("Quit ?"));

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


	/*if(ImGui::IsKeyDown(int(Key::P))) {
		y_fatal("Crash!");
	}*/
}

void MainWindow::render_ui(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	// demo
	ImGui::ShowDemoWindow();

	// menu
	{
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("View")) {
				if(ImGui::MenuItem("Engine view"))		show_element<EngineView>(context(), _elements);
				if(ImGui::MenuItem("Entities"))			show_element<EntityView>(context(), _elements);
				if(ImGui::MenuItem("Asset browser"))	show_element<AssetBrowser>(context(), _elements);
				if(ImGui::MenuItem("Properties"))		show_element<PropertyPanel>(context(), _elements);
				if(ImGui::MenuItem("Settings"))			show_element<SettingsPanel>(context(), _elements);


				if(ImGui::BeginMenu("Debug")) {
					if(ImGui::MenuItem("Camera debug")) show_element<CameraDebug>(context(), _elements);
					if(ImGui::MenuItem("Scene debug"))	show_element<SceneDebug>(context(), _elements);
					ImGui::EndMenu();
				}
				if(ImGui::BeginMenu("Statistics")) {
					if(ImGui::MenuItem("Performances")) show_element<PerformanceMetrics>(context(), _elements);
					if(ImGui::MenuItem("Memory info"))	show_element<MemoryInfo>(context(), _elements);
					ImGui::EndMenu();
				}


				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	// toolbar
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));

		float toolbar_size = 18.0f;
		ImVec2 button_size(toolbar_size, toolbar_size);

		if(ImGui::ImageButton(&context()->icons()->save, button_size)) {
			//context()->save_scene(filename);
			show_element<FileBrowser>(context(), _elements)->set_callback(
					[=](core::String filename) { context()->save_scene(filename); }
				);

		}
		ImGui::SameLine(0.0f, 0.1f);
		if(ImGui::ImageButton(&context()->icons()->load, button_size)) {
			//context()->load_scene(filename);
			show_element<FileBrowser>(context(), _elements)->set_callback(
					[=](core::String filename) { context()->defer([=] { context()->load_scene(filename); }); }
				);
		}
		ImGui::PopStyleVar();
	}

	// main UI
	{
		ImGui::BeginDockspace();
		for(auto& e : _elements) {
			e->paint(recorder, token);
		}
		ImGui::EndDockspace();
	}
}

}

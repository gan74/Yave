/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <editor/widgets/EntityView.h>
#include <editor/widgets/FileBrowser.h>
#include <editor/widgets/PropertyPanel.h>
#include <editor/widgets/SettingsPanel.h>
#include <editor/widgets/CameraDebug.h>
#include <editor/widgets/MemoryInfo.h>
#include <editor/widgets/PerformanceMetrics.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/widgets/MaterialEditor.h>

#include <editor/widgets/AssetStringifier.h>
#include <editor/widgets/EcsDebug.h>

#include <editor/EngineView.h>

#include <imgui/yave_imgui.h>

namespace editor {

MainWindow::MainWindow(ContextPtr cptr) :
		Window({1280, 768}, "Yave", Window::Resizable),
		ContextLinked(cptr) {

	_ui_renderer = std::make_unique<ImGuiRenderer>(device());

	set_event_handler(std::make_unique<MainEventHandler>());

	context()->ui().show<EngineView>();
	context()->ui().show<EntityView>();
	context()->ui().show<ResourceBrowser>();
	context()->ui().show<PropertyPanel>();
	context()->ui().show<EcsDebug>();
}

MainWindow::~MainWindow() {
}

void MainWindow::resized() {
	create_swapchain();
}

void MainWindow::create_swapchain() {
	// needed because the swapchain immediatly destroys it images
	device()->wait_all_queues();

	if(_swapchain) {
		_swapchain->reset();
	} else {
		_swapchain = std::make_unique<Swapchain>(device(), static_cast<Window*>(this));
	}

	_framebuffers = std::make_unique<Framebuffer[]>(_swapchain->image_count());
	for(usize i = 0; i != _swapchain->image_count(); ++i) {
		ColorAttachmentView color(_swapchain->images()[i]);
		_framebuffers[i] = Framebuffer(device(), {color});
	}
}

void MainWindow::exec() {
	do {
		show();
		while(update()) {
			core::DebugTimer _("Frame time", core::Duration::milliseconds(30));

			if(_swapchain->size().x() && _swapchain->size().y()) {
				FrameToken frame = _swapchain->next_frame();
				CmdBufferRecorder recorder(device()->create_disposable_cmd_buffer());

				render(recorder, frame);
				present(recorder, frame);
			}
		}
	} while(!context()->ui().confirm("Quit ?"));

	device()->wait_all_queues();
}

void MainWindow::present(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();
	{
		RecordedCmdBuffer cmd_buffer(std::move(recorder));

		vk::PipelineStageFlags pipe_stage_flags = vk::PipelineStageFlagBits::eBottomOfPipe;
		Y_TODO(manual locking needs for queue presentation needs to go)
		const auto& queue = device()->graphic_queue();
		std::unique_lock lock(queue.lock());
		auto graphic_queue = queue.vk_queue();
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

	context()->flush_deferred();
}

void MainWindow::render(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();

	Y_TODO(move ui code out of main window)

	ImGui::GetIO().DisplaySize = math::Vec2(_swapchain->size());

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoDocking |
							 ImGuiWindowFlags_NoTitleBar |
							 ImGuiWindowFlags_NoCollapse |
							 ImGuiWindowFlags_NoResize |
							 ImGuiWindowFlags_NoMove |
							 ImGuiWindowFlags_NoBringToFrontOnFocus |
							 ImGuiWindowFlags_NoNavFocus;


	ImGui::NewFrame();

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_MenuBar | flags);

	ImGui::DockSpace(ImGui::GetID("Dockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

	ImGui::PopStyleVar(3);

	render_ui(recorder, token);

	ImGui::End();
	ImGui::EndFrame();
	ImGui::Render();


	// render ui pipeline into cmd buffer
	{
		auto render_pass = recorder.bind_framebuffer(_framebuffers[token.image_index]);
		_ui_renderer->render(render_pass, token);
	}
}

void MainWindow::render_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();

	// menu
	{
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu(ICON_FA_FILE " File")) {

				if(ImGui::MenuItem(ICON_FA_FILE " New")) {
					context()->new_world();
				}

				ImGui::Separator();

				if(ImGui::MenuItem(ICON_FA_SAVE " Save")) {
					context()->defer([ctx = context()] { ctx->save_world(); });
					/*FileBrowser* browser = context()->ui().show<FileBrowser>();
					browser->set_extension_filter("*.ys");
					browser->set_selected_callback(
							[ctx = context()](const auto& filename) { ctx->scene().save(filename); return true; }
						);*/
				}

				if(ImGui::MenuItem(ICON_FA_FOLDER " Load")) {
					context()->load_world();
				}

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("View")) {
				if(ImGui::MenuItem("Engine view")) context()->ui().add<EngineView>();
				if(ImGui::MenuItem("Entity view")) context()->ui().add<EntityView>();
				if(ImGui::MenuItem("Resource browser")) context()->ui().add<ResourceBrowser>();
				if(ImGui::MenuItem("Material editor")) context()->ui().add<MaterialEditor>();

				ImGui::Separator();

				if(ImGui::BeginMenu("Debug")) {
					if(ImGui::MenuItem("Camera debug")) context()->ui().add<CameraDebug>();
					if(ImGui::MenuItem("ECS debug")) context()->ui().add<EcsDebug>();

					ImGui::Separator();
					if(ImGui::MenuItem("Asset stringifier")) context()->ui().add<AssetStringifier>();

					ImGui::Separator();
					if(ImGui::MenuItem("Flush reload")) context()->flush_reload();

					y_debug_assert(!(ImGui::Separator(), ImGui::MenuItem("Debug assert")));

					ImGui::EndMenu();
				}
				if(ImGui::BeginMenu("Statistics")) {
					if(ImGui::MenuItem("Performances")) context()->ui().add<PerformanceMetrics>();
					if(ImGui::MenuItem("Memory info")) context()->ui().add<MemoryInfo>();
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if(ImGui::MenuItem("Settings")) context()->ui().add<SettingsPanel>();

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}


	if(context()->selection().has_selected()) {
		context()->ui().show<PropertyPanel>();
	}

	context()->ui().paint(recorder, token);

	// demo
	ImGui::ShowDemoWindow();

}

}

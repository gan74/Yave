/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "Ui.h"

#include <editor/context/EditorContext.h>

#include <editor/widgets/EntityView.h>
#include <editor/widgets/ResourceBrowser.h>
#include <editor/widgets/Console.h>

#include <editor/properties/PropertyPanel.h>
#include <editor/EngineView.h>
#include <editor/ui/MenuBar.h>

#include <editor/ui/ImGuiRenderer.h>
#include <imgui/yave_imgui.h>

namespace editor {

Ui::Ui(ContextPtr ctx) : ContextLinked(ctx) {
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = "editor.ini";
	ImGui::GetIO().LogFilename = "editor_logs.txt";
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().ConfigDockingWithShift = false;
	//ImGui::GetIO().ConfigResizeWindowsFromEdges = true;

	show<EngineView>();
	show<EntityView>();
	show<Console>();
	show<ResourceBrowser>();
	show<PropertyPanel>();
	show<MenuBar>();

	_renderer = std::make_unique<ImGuiRenderer>(context());
}

Ui::~Ui() {
	ImGui::DestroyContext();
}

const ImGuiRenderer& Ui::renderer() const {
	return *_renderer;
}

core::Span<std::unique_ptr<UiElement>> Ui::ui_elements() const {
	return _elements;
}

bool Ui::confirm(const char* message) {
#ifdef Y_OS_WIN
	return MessageBox(GetActiveWindow(), message, "Confirm", MB_OKCANCEL) != IDCANCEL;
#else
#warning not supported
	return true;
#endif
}

void Ui::ok(const char* title, const char* message) {
#ifdef Y_OS_WIN
	MessageBox(GetActiveWindow(), message, title, MB_OK);
#else
#warning not supported
#endif
}

void Ui::refresh_all() {
	for(auto&& elem : _elements) {
		elem->refresh();
	}
}

Ui::Ids& Ui::ids_for(UiElement* elem) {
	return _ids[typeid(*elem)];
}

void Ui::set_id(UiElement* elem) {
	auto& ids = ids_for(elem);
	if(!ids.released.is_empty()) {
		elem->set_id(ids.released.pop());
	} else {
		elem->set_id(ids.next++);
	}
}




void Ui::paint(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();

	ImGui::GetIO().DeltaTime = float(_frame_timer.reset().to_secs());
	ImGui::GetIO().DisplaySize = token.image_view.size();

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

	paint_ui(recorder, token);

	ImGui::End();
	ImGui::Render();

	{
		y_profile_zone("ui render");
		Framebuffer framebuffer(token.image_view.device(), {token.image_view});
		RenderPassRecorder pass = recorder.bind_framebuffer(framebuffer);
		_renderer->render(pass, token);
	}
}

void Ui::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();
	// demo
	ImGui::ShowDemoWindow();

	for(auto& e : _elements) {
		e->paint(recorder, token);
	}

	for(usize i = 0; i < _elements.size();) {
		if(!_elements[i]->is_visible()) {
			ids_for(_elements[i].get()).released << _elements[i]->_id;
			_elements.erase_unordered(_elements.begin() + i);
		} else {
			++i;
		}
	}
}

}

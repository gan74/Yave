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

#include "Ui.h"

#include <imgui/yave_imgui.h>

namespace editor {

Ui::Ui(ContextPtr ctx) : ContextLinked(ctx) {
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = "editor.ini";
	ImGui::GetIO().LogFilename = "editor_logs.txt";
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::GetIO().ConfigDockingWithShift = false;
	//ImGui::GetIO().ConfigResizeWindowsFromEdges = true;
}

Ui::~Ui() {
	ImGui::DestroyContext();
}

core::ArrayView<std::unique_ptr<Widget>> Ui::widgets() const {
	return _widgets;
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

void Ui::paint(CmdBufferRecorder& recorder, const FrameToken& token) {
	y_profile();
	for(auto& e : _widgets) {
		e->paint(recorder, token);
	}

	for(usize i = 0; i < _widgets.size();) {
		if(!_widgets[i]->is_visible()) {
			ids_for(_widgets[i].get()).released << _widgets[i]->_id;
			_widgets.erase_unordered(_widgets.begin() + i);
		} else {
			++i;
		}
	}
}

void Ui::refresh_all() {
	for(auto&& widget : _widgets) {
		widget->refresh();
	}
}

Ui::Ids& Ui::ids_for(Widget* widget) {
	return _ids[typeid(*widget)];
}

void Ui::set_id(Widget* widget) {
	auto& ids = ids_for(widget);
	if(!ids.released.is_empty()) {
		widget->set_id(ids.released.pop());
	} else {
		widget->set_id(ids.next++);
	}
}

}

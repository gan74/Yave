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

#include "Widget.h"

#include <imgui/imgui.h>

namespace editor {

Widget::Widget(std::string_view title, u32 flags) : UiElement(title), _flags(flags) {
}

const math::Vec2& Widget::position() const {
	y_debug_assert(!_has_parent);
	return _position;
}

const math::Vec2& Widget::size() const {
	y_debug_assert(!_has_parent);
	return _size;
}

void Widget::set_has_parent(bool has) {
	_has_parent = has;
}


void Widget::set_closable(bool closable) {
	_closable = closable;
}

void Widget::paint(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	if(!is_visible()) {
		return;
	}

	if(_has_parent) {
		if(ImGui::BeginChild(_title_with_id.begin(), ImVec2(0, 0), false, _flags)) {
			paint_ui(recorder, token);
		}
		ImGui::EndChild();
	} else {
		if(ImGui::Begin(_title_with_id.begin(), _closable ? &_visible : nullptr, _flags)) {
			_position = ImGui::GetWindowPos();
			_size = ImGui::GetWindowSize();
			paint_ui(recorder, token);
		}
		ImGui::End();
	}
}

}

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
#ifndef EDITOR_UI_WIDGET_H
#define EDITOR_UI_WIDGET_H

#include "UiElement.h"

namespace editor {

class Widget : public UiElement {

	public:
		Widget(std::string_view title, u32 flags = 0);

		void paint(CmdBufferRecorder& recorder, const FrameToken& token) override;

		const math::Vec2& position() const;
		const math::Vec2& size() const;

		void set_has_parent(bool has);

	protected:
		virtual void paint_ui(CmdBufferRecorder&, const FrameToken&) = 0;

		void set_closable(bool closable);

		math::Vec2ui content_size() const;

	private:
		void update_attribs();

		math::Vec2 _position;
		math::Vec2 _size;

		math::Vec2 _min_size;

		u32 _flags;
		bool _closable = true;
		bool _has_parent = false;
		bool _docked = false;
};

}

#endif // EDITOR_UI_WIDGET_H

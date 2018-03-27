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
#ifndef EDITOR_EVENTS_MAINEVENTHANDLER_H
#define EDITOR_EVENTS_MAINEVENTHANDLER_H

#include <editor/editor.h>

#include <yave/window/EventHandler.h>

namespace editor {

class MainEventHandler : public EventHandler
{
	public:
		MainEventHandler();

		virtual ~MainEventHandler();

		void mouse_moved(const math::Vec2i& pos) override;
		void mouse_pressed(const math::Vec2i& pos, MouseButton button) override;
		void mouse_released(const math::Vec2i& pos, MouseButton button) override;

		void mouse_wheel(int delta) override;

		void char_input(u32 character) override;

		void key_pressed(Key key) override;
		void key_released(Key key) override;


	private:
};

}

#endif // EDITOR_EVENTS_MAINEVENTHANDLER_H

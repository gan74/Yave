/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef YAVE_WINDOW_H
#define YAVE_WINDOW_H

#include "yave.h"
#include "EventHandler.h"

#include <y/core/String.h>
#include <y/core/Ptr.h>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

namespace yave {

class Window {
	public:
		Window(const math::Vec2ui& _size, const core::String& _name);
		~Window();

		void close();
		bool update();

		void show();

		#ifdef Y_OS_WIN
			HINSTANCE instance() const { return _hInstance; }
			HWND handle() const { return _hwnd; }
		#endif

		math::Vec2ui size() const;

		void set_mouse_handler(MouseEventHandler*&& handler);
		MouseEventHandler* mouse_handler() const;

	private:
		#ifdef Y_OS_WIN
			HINSTANCE _hInstance;
			HWND _hwnd;
			bool _run;
		#endif

		math::Vec2ui _size;
		core::String _name;

		mutable core::Unique<MouseEventHandler> _mouse_handler;
};

}

#endif // YAVE_WINDOW_H

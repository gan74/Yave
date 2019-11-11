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
#ifndef YAVE_WINDOW_WINDOW_H
#define YAVE_WINDOW_WINDOW_H

#include <yave/yave.h>
#include "EventHandler.h"

#include <y/core/String.h>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

namespace yave {

class Window : NonMovable {
	public:
		enum Flags {
			NoFlags = 0,
			Resizable = 0x01
		};


		Window(const math::Vec2ui& size, const core::String& name, Flags flags = NoFlags);
		virtual ~Window();

		void close();
		bool update();

		void show();

		#ifdef Y_OS_WIN
			HINSTANCE instance() const { return _hinstance; }
			HWND handle() const { return _hwnd; }
		#endif

		const math::Vec2ui& size() const;
		math::Vec2ui position() const;

		void set_event_handler(std::unique_ptr<EventHandler> handler);
		EventHandler* event_handler() const;

	protected:
		virtual void resized() {
		}

	private:
		#ifdef Y_OS_WIN
			static LRESULT CALLBACK windows_event_handler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
			static void mouse_event(Window* window, UINT uMsg, POINTS pt);
			HINSTANCE _hinstance;
			HWND _hwnd;
			bool _run;
		#endif

		math::Vec2ui _size;
		core::String _name;

		mutable std::unique_ptr<EventHandler> _event_handler;
};

}

#endif // YAVE_WINDOW_WINDOW_H

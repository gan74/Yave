/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
		MouseEventHandler* get_mouse_handler() const;

	private:
		#ifdef Y_OS_WIN
			HINSTANCE _hInstance;
			HWND _hwnd;
			bool _run;
		#endif

		math::Vec2ui _size;
		core::String _name;

		mutable core::Ptr<MouseEventHandler> _mouse_handler;
};

}

#endif // YAVE_WINDOW_H

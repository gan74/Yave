/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#include "Window.h"

namespace yave {

#ifdef Y_OS_WIN

const char class_name[] = "Yave";

void mouse_event(Window* window, UINT uMsg, POINTS pt) {
	if(window->get_mouse_handler()) {
		auto handler = window->get_mouse_handler();
		auto pos = math::vec(pt.x, window->size().y() - usize(pt.y));
		switch(uMsg) {
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
				handler->mouse_pressed(pos, uMsg == WM_LBUTTONDOWN ? MouseEventHandler::LeftButton : MouseEventHandler::RightButton);
				break;

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
				handler->mouse_released(pos, uMsg == WM_LBUTTONUP ? MouseEventHandler::LeftButton : MouseEventHandler::RightButton);
				break;

			case WM_MOUSEMOVE:
				handler->mouse_moved(pos);
				break;

			default:
				break;
		}
	}
}

LRESULT CALLBACK windows_event_handler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	auto& l_param = lParam;
	switch(uMsg) {
		case WM_CLOSE:
			window->close();
			return 0;

		case WM_SIZE:
			// we get here if the window has changed size, we should rebuild most
			// of our window resources before rendering to this window again.
			// (no need for this because our window sizing by hand is disabled)
			break;

		case WM_KEYDOWN:
			if(wParam == VK_ESCAPE) {
				window->close();
				return 0;
			}
			break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MOUSEMOVE:
			mouse_event(window, uMsg, reinterpret_cast<const POINTS&>(l_param));
			break;

		default:
			break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#endif



Window::Window(const math::Vec2ui& win_size, const core::String& win_name) : _size(win_size), _name(win_name), _mouse_handler(nullptr) {
	#ifdef Y_OS_WIN
		_run = true;
		_hInstance = GetModuleHandle(nullptr);
		WNDCLASSEX wc = {};
		wc.cbSize        = sizeof(WNDCLASSEX);
		wc.style		 = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = windows_event_handler;
		wc.hInstance     = _hInstance;
		wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
		wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = HBRUSH(COLOR_WINDOW + 1);
		wc.lpszClassName = class_name;
		wc.hIconSm       = LoadIcon(nullptr, IDI_APPLICATION);
		RegisterClassEx(&wc);


		DWORD ex_style = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD style	  = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
		RECT wr = {0, 0, LONG(win_size.x()), LONG(win_size.y())};
		AdjustWindowRectEx(&wr, style, FALSE, ex_style);

		_hwnd = CreateWindowEx(0, class_name, win_name.data(), style, CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, _hInstance, nullptr);
		SetWindowLongPtr(_hwnd, GWLP_USERDATA, LONG_PTR(this));
	#endif
}

Window::~Window() {
	#ifdef Y_OS_WIN
		UnregisterClass(class_name, _hInstance);
	#endif
}

void Window::close() {
	#ifdef Y_OS_WIN
		_run = false;
	#endif
}

bool Window::update() {
	#ifdef Y_OS_WIN
		MSG msg;
		while(PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return _run;
	#endif
}

void Window::show() {
	#ifdef Y_OS_WIN
		ShowWindow(_hwnd, SW_SHOW);
		SetForegroundWindow(_hwnd);
		SetFocus(_hwnd);
	#endif
}

math::Vec2ui Window::size() const {
	return _size;
}

void Window::set_mouse_handler(MouseEventHandler*&& handler) {
	_mouse_handler = std::move(handler);
}

MouseEventHandler* Window::get_mouse_handler() const {
	return _mouse_handler.as_ptr();
}

}

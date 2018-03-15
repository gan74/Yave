/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include "Window.h"

namespace yave {

#ifdef Y_OS_WIN

const char class_name[] = "Yave";

void mouse_event(Window* window, UINT uMsg, POINTS pt) {
	if(window->event_handler()) {
		auto handler = window->event_handler();
		math::Vec2i pos = math::Vec2i(pt.x, pt.y);
		switch(uMsg) {
			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
				handler->mouse_pressed(pos, uMsg == WM_LBUTTONDOWN ? EventHandler::LeftButton : EventHandler::RightButton);
				break;

			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
				handler->mouse_released(pos, uMsg == WM_LBUTTONUP ? EventHandler::LeftButton : EventHandler::RightButton);
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
			[[fallthrough]];

		case WM_KEYUP:
			if(auto handler = window->event_handler(); handler) {
				uMsg == WM_KEYDOWN
					? handler->key_pressed(u32(wParam))
					: handler->key_released(u32(wParam));
			}
			break;

		case WM_CHAR:
			if(auto handler = window->event_handler(); handler) {
				handler->char_input(u32(wParam));
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



Window::Window(const math::Vec2ui& win_size, const core::String& win_name) : _size(win_size), _name(win_name), _event_handler(nullptr) {
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

const math::Vec2ui& Window::size() const {
	return _size;
}

math::Vec2ui Window::position() const {
#ifdef Y_OS_WIN
	RECT rect;
	GetWindowRect(_hwnd, &rect);
	return math::Vec2ui(rect.left, 0);
#endif
}

void Window::set_event_handler(EventHandler*&& handler) {
	_event_handler = std::move(handler);
}

EventHandler* Window::event_handler() const {
	return _event_handler.as_ptr();
}

}

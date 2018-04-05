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


static Key to_key(WPARAM param) {
	char c = char(param);
	if(std::isalpha(c)) {
		return Key(std::toupper(c));
	}
	switch(param) {
		case VK_TAB:	return Key::Tab;
		case VK_CLEAR:	return Key::Clear;
		case VK_BACK:	return Key::Backspace;
		case VK_RETURN: return Key::Enter;
		case VK_ESCAPE: return Key::Escape;
		case VK_PRIOR:	return Key::PageUp;
		case VK_NEXT:	return Key::PageDown;
		case VK_END:	return Key::End;
		case VK_HOME:	return Key::Home;
		case VK_LEFT:	return Key::Left;
		case VK_RIGHT:	return Key::Right;
		case VK_UP:		return Key::Up;
		case VK_DOWN:	return Key::Down;
		case VK_INSERT: return Key::Insert;
		case VK_DELETE: return Key::Delete;
		case VK_SPACE:	return Key::Space;

		default:
			break;
	}
	return Key::Unknown;
}

LRESULT CALLBACK Window::windows_event_handler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if(window) {
		auto& l_param = lParam;
		switch(uMsg) {
			case WM_CLOSE:
				window->close();
				return 0;

			case WM_SIZE:
				window->_size = math::Vec2ui(usize(LOWORD(lParam)), usize(HIWORD(lParam)));
				window->resized();
				return 0;

			case WM_KEYDOWN:
			case WM_KEYUP:
				if(auto handler = window->event_handler()) {
					auto k = to_key(wParam);
					if(k != Key::Unknown) {
						uMsg == WM_KEYDOWN
							? handler->key_pressed(k)
							: handler->key_released(k);
						return 0;
					}
				} else if(uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
					// escape close the window by default
					window->close();
					return 0;
				}
				break;

			case WM_CHAR:
				if(auto handler = window->event_handler()) {
					handler->char_input(u32(wParam));
					return 0;
				}
				break;

			case WM_LBUTTONDOWN:
			case WM_RBUTTONDOWN:
			case WM_MBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
			case WM_MBUTTONUP:
			case WM_MOUSEMOVE:
			case WM_MOUSEWHEEL:
				if(auto handler = window->event_handler()) {
					auto pt = reinterpret_cast<const POINTS&>(l_param);
					math::Vec2i pos = math::Vec2i(pt.x, pt.y);
					switch(uMsg) {
						case WM_LBUTTONDOWN:
							handler->mouse_pressed(pos, EventHandler::LeftButton);
							return 0;

						case WM_RBUTTONDOWN:
							handler->mouse_pressed(pos, EventHandler::RightButton);
							return 0;

						case WM_MBUTTONDOWN:
							handler->mouse_pressed(pos, EventHandler::MiddleButton);
							return 0;

						case WM_LBUTTONUP:
							handler->mouse_released(pos, EventHandler::LeftButton);
							return 0;

						case WM_RBUTTONUP:
							handler->mouse_released(pos, EventHandler::RightButton);
							return 0;

						case WM_MBUTTONUP:
							handler->mouse_released(pos, EventHandler::MiddleButton);
							return 0;

						case WM_MOUSEMOVE:
							handler->mouse_moved(pos);
							return 0;

						case WM_MOUSEWHEEL:
							handler->mouse_wheel(int(i16(HIWORD(wParam)) / WHEEL_DELTA));
							return 0;

						default:
							break;
					}
				}
				break;

			default:
				break;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#endif



Window::Window(const math::Vec2ui& size, const core::String& name, Flags flags) : _size(size), _name(name), _event_handler(nullptr) {
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
	DWORD style	= WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	if(flags & Resizable) {
		style |= WS_SIZEBOX | WS_MAXIMIZEBOX;
	}

	RECT wr = {0, 0, LONG(size.x()), LONG(size.y())};
	AdjustWindowRectEx(&wr, style, FALSE, ex_style);

	_hwnd = CreateWindowEx(0, class_name, name.data(), style, CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top, nullptr, nullptr, _hInstance, nullptr);
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
	_event_handler = std::unique_ptr<EventHandler>(std::move(handler));
}

EventHandler* Window::event_handler() const {
	return _event_handler.get();
}

}

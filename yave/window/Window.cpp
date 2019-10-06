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
#include "Window.h"

namespace yave {

#ifdef Y_OS_WIN

static const char class_name[] = "Yave";

static u32 is_ascii(WPARAM w_param, LPARAM l_param) {
	//https://stackoverflow.com/questions/44660035/how-to-extract-the-character-from-wm-keydown-in-pretranslatemessagemsgpmsg
	static BYTE kb_state[256];
	static auto init = GetKeyboardState(kb_state);
	unused(init);
	auto scan_code = (l_param >> 16) & 0xFF;
	WORD ascii = 0;
	return ToAscii(w_param, scan_code, kb_state, &ascii, 0);
}


static Key to_key(WPARAM w_param, LPARAM l_param) {
	if(!std::iscntrl(w_param) && is_ascii(w_param, l_param)) {
		return Key(std::toupper(w_param));
	}
	switch(w_param) {
		case VK_TAB:		return Key::Tab;
		case VK_CLEAR:		return Key::Clear;
		case VK_BACK:		return Key::Backspace;
		case VK_RETURN:		return Key::Enter;
		case VK_ESCAPE:		return Key::Escape;
		case VK_PRIOR:		return Key::PageUp;
		case VK_NEXT:		return Key::PageDown;
		case VK_END:		return Key::End;
		case VK_HOME:		return Key::Home;
		case VK_LEFT:		return Key::Left;
		case VK_RIGHT:		return Key::Right;
		case VK_UP:			return Key::Up;
		case VK_DOWN:		return Key::Down;
		case VK_INSERT:		return Key::Insert;
		case VK_DELETE:		return Key::Delete;
		case VK_SPACE:		return Key::Space;
		case VK_F1:			return Key::F1;
		case VK_F2:			return Key::F2;
		case VK_F3:			return Key::F3;
		case VK_F4:			return Key::F4;
		case VK_F5:			return Key::F5;
		case VK_F6:			return Key::F6;
		case VK_F7:			return Key::F7;
		case VK_F8:			return Key::F8;
		case VK_F9:			return Key::F9;
		case VK_F10:		return Key::F10_Reserved;
		case VK_F11:		return Key::F11;
		case VK_F12:		return Key::F12;
		case VK_MENU:		return Key::Alt;
		case VK_CONTROL:	return Key::Ctrl;

		default:
			break;
	}
	return Key::Unknown;
}

LRESULT CALLBACK Window::windows_event_handler(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
	Window* window = reinterpret_cast<Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	if(window) {
		math::Vec2ui lvec(usize(LOWORD(l_param)), usize(HIWORD(l_param)));
		switch(u_msg) {
			case WM_CLOSE:
				window->close();
				return 0;

			case WM_SIZE:
				window->_size = lvec;
				window->resized();
				return 0;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP: {
				bool is_down = u_msg == WM_SYSKEYDOWN ||
							   u_msg == WM_KEYDOWN;
				/*bool is_system = u_msg == WM_SYSKEYDOWN ||
								 u_msg == WM_SYSKEYUP;*/
				if(auto handler = window->event_handler()) {
					auto k = to_key(w_param, l_param);
					if(k != Key::Unknown) {
						is_down
							? handler->key_pressed(k)
							: handler->key_released(k);
						return 0;
					}
				} else if(is_down && w_param == VK_ESCAPE) {
					// escape close the window by default
					window->close();
					return 0;
				}
			} break;

			case WM_CHAR:
				if(auto handler = window->event_handler()) {
					handler->char_input(u32(w_param));
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
					switch(u_msg) {
						case WM_LBUTTONDOWN:
							handler->mouse_pressed(lvec, EventHandler::LeftButton);
							return 0;

						case WM_RBUTTONDOWN:
							handler->mouse_pressed(lvec, EventHandler::RightButton);
							return 0;

						case WM_MBUTTONDOWN:
							handler->mouse_pressed(lvec, EventHandler::MiddleButton);
							return 0;

						case WM_LBUTTONUP:
							handler->mouse_released(lvec, EventHandler::LeftButton);
							return 0;

						case WM_RBUTTONUP:
							handler->mouse_released(lvec, EventHandler::RightButton);
							return 0;

						case WM_MBUTTONUP:
							handler->mouse_released(lvec, EventHandler::MiddleButton);
							return 0;

						case WM_MOUSEMOVE:
							handler->mouse_moved(lvec);
							return 0;

						case WM_MOUSEWHEEL:
							handler->mouse_wheel(i32(i16(HIWORD(w_param)) / WHEEL_DELTA));
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
	return DefWindowProc(hwnd, u_msg, w_param, l_param);
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
	_run = true;
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

void Window::set_event_handler(std::unique_ptr<EventHandler> handler) {
	_event_handler = std::move(handler);
}

EventHandler* Window::event_handler() const {
	return _event_handler.get();
}

}

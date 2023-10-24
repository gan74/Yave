/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#ifdef Y_OS_WIN

#include <cctype>

#include <windows.h>
#include <winuser.h>

namespace yave {

static u32 is_ascii(WPARAM w_param, LPARAM l_param) {
    //https://stackoverflow.com/questions/44660035/how-to-extract-the-character-from-wm-keydown-in-pretranslatemessagemsgpmsg
    static BYTE kb_state[256];
    static auto init = ::GetKeyboardState(kb_state);
    unused(init);
    const auto scan_code = (l_param >> 16) & 0xFF;
    WORD ascii = 0;
    return ::ToAscii(UINT(w_param), UINT(scan_code), kb_state, &ascii, 0);
}

static Key to_key(WPARAM w_param, LPARAM l_param) {
    if(!std::iscntrl(int(w_param)) && is_ascii(w_param, l_param)) {
        const Key key = Key(std::toupper(int(w_param)));
        return is_character_key(key) ? key : Key::Unknown;
    }

    switch(w_param) {
        case VK_TAB:        return Key::Tab;
        case VK_CLEAR:      return Key::Clear;
        case VK_BACK:       return Key::Backspace;
        case VK_RETURN:     return Key::Enter;
        case VK_ESCAPE:     return Key::Escape;
        case VK_PRIOR:      return Key::PageUp;
        case VK_NEXT:       return Key::PageDown;
        case VK_END:        return Key::End;
        case VK_HOME:       return Key::Home;
        case VK_LEFT:       return Key::Left;
        case VK_RIGHT:      return Key::Right;
        case VK_UP:         return Key::Up;
        case VK_DOWN:       return Key::Down;
        case VK_INSERT:     return Key::Insert;
        case VK_DELETE:     return Key::Delete;
        case VK_SPACE:      return Key::Space;
        case VK_F1:         return Key::F1;
        case VK_F2:         return Key::F2;
        case VK_F3:         return Key::F3;
        case VK_F4:         return Key::F4;
        case VK_F5:         return Key::F5;
        case VK_F6:         return Key::F6;
        case VK_F7:         return Key::F7;
        case VK_F8:         return Key::F8;
        case VK_F9:         return Key::F9;
        case VK_F10:        return Key::F10_Reserved;
        case VK_F11:        return Key::F11;
        case VK_F12:        return Key::F12;
        case VK_MENU:       return Key::Alt;
        case VK_CONTROL:    return Key::Ctrl;

        default:
            break;
    }
    return Key::Unknown;
}

static bool mouse_button_down(UINT u_msg) {
    switch(u_msg) {
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
            return true;

        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
            return false;

        default:
        break;
    }
    y_fatal("Unknown mouse button");
}

static MouseButton mouse_button(UINT u_msg) {
    switch(u_msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return MouseButton::LeftButton;

        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return MouseButton::RightButton;

        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return MouseButton::MiddleButton;

        default:
        break;
    }
    y_fatal("Unknown mouse button");
}

static LPTSTR to_windows_cursor_icon(CursorShape shape) {
    switch(shape) {
        case CursorShape::Arrow:
            return IDC_ARROW;
        case CursorShape::TextInput:
            return IDC_IBEAM;
        case CursorShape::ResizeAll:
            return IDC_SIZEALL;
        case CursorShape::ResizeEW:
            return IDC_SIZEWE;
        case CursorShape::ResizeNS:
            return IDC_SIZENS;
        case CursorShape::ResizeNESW:
            return IDC_SIZENESW;
        case CursorShape::ResizeNWSE:
            return IDC_SIZENWSE;
        case CursorShape::Hand:
            return IDC_HAND;
        case CursorShape::NotAllowed:
            return IDC_NO;

        default:
            return IDC_ARROW;
    }
}

[[maybe_unused]]
static math::Vec2i client_mouse_pos(Window* win) {
    POINT mouse_pos = {};
    ::GetCursorPos(&mouse_pos);
    ::ScreenToClient(win->handle(), &mouse_pos);
    return math::Vec2i(mouse_pos.x, mouse_pos.y);
}


void notify_resized(Window* window) {
    window->resized();
}

void update_capture_state(Window* win, bool button_down, MouseButton button) {
    const u32 mask = 1 << u32(button);

    if(button_down) {
        win->_mouse_state |= mask;
        if(!::GetCapture()) {
            ::SetCapture(win->handle());
        }
    } else {
        win->_mouse_state &= ~mask;
        if(!win->_mouse_state && ::GetCapture() == win->handle()) {
            ::ReleaseCapture();
        }
    }
}

static LRESULT CALLBACK windows_event_handler(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
    Window* window = reinterpret_cast<Window*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if(window) {
        const i16* l_params = reinterpret_cast<const i16*>(&l_param);
        const math::Vec2i lvec(l_params[0], l_params[1]);
        switch(u_msg) {
            case WM_CLOSE:
                window->close();
                return 0;

            case WM_SIZE:
                notify_resized(window);
                return 0;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                const bool is_down =
                    u_msg == WM_SYSKEYDOWN ||
                    u_msg == WM_KEYDOWN;

                if(const auto handler = window->event_handler()) {
                    const auto k = to_key(w_param, l_param);
                    if(k != Key::Unknown) {
                        is_down ? handler->key_pressed(k) : handler->key_released(k);
                        return 0;
                    }
                } else if(is_down && w_param == VK_ESCAPE) {
                    // escape close the window by default
                    window->close();
                    return 0;
                }
            } break;

            case WM_CHAR:
                if(const auto handler = window->event_handler()) {
                    handler->char_input(u32(w_param));
                    return 0;
                }
            break;

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
            case WM_MBUTTONUP: {
                const bool down = mouse_button_down(u_msg);
                const MouseButton button = mouse_button(u_msg);
                update_capture_state(window, down, button);
                if(const auto handler = window->event_handler()) {
                    down ? handler->mouse_pressed(lvec, button) : handler->mouse_released(lvec, button);
                    return 0;
                }
             } break;

            case WM_MOUSEMOVE:
                if(const auto handler = window->event_handler()) {
                    handler->mouse_moved(lvec);
                    return 0;
                }
            break;

            case WM_MOUSEWHEEL:
            case WM_MOUSEHWHEEL:
                if(const auto handler = window->event_handler()) {
                    const i32 delta = i32(i16(HIWORD(w_param)) / WHEEL_DELTA);
                    const bool v = u_msg == WM_MOUSEWHEEL;
                    handler->mouse_wheel(v ? delta : 0, v ? 0 : delta);
                    return 0;
                }
            break;

            default:
            break;
        }
    }

    y_profile_zone("windows proc");
    return ::DefWindowProc(hwnd, u_msg, w_param, l_param);
}


static const char windows_class_name[] = "yave_window";

static void register_window_class() {
    static bool registered = false;

    if(!registered) {
        WNDCLASSEX win_class = {};
        win_class.cbSize            = sizeof(WNDCLASSEX);
        win_class.style             = CS_HREDRAW | CS_VREDRAW;
        win_class.lpfnWndProc       = windows_event_handler;
        win_class.hInstance         = ::GetModuleHandle(nullptr);
        win_class.hIcon             = ::LoadIcon(nullptr, IDI_APPLICATION);
        win_class.hCursor           = ::LoadCursor(nullptr, IDC_ARROW);
        win_class.hbrBackground     = HBRUSH(COLOR_WINDOW + 1);
        win_class.lpszClassName     = windows_class_name;
        win_class.hIconSm           = ::LoadIcon(nullptr, IDI_APPLICATION);
        ::RegisterClassEx(&win_class);

        registered = true;
    }
}



Window::Window(const math::Vec2ui& size, const core::String& title, Flags flags) {
    register_window_class();

    _hinstance = ::GetModuleHandle(nullptr);

    _ex_style = WS_EX_APPWINDOW;
    _style = (flags & NoDecoration)
        ? WS_POPUP
        : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    if(flags & Resizable) {
        _style |= WS_SIZEBOX | WS_MAXIMIZEBOX;
    }

    RECT rect = {0, 0, LONG(size.x()), LONG(size.y())};
    ::AdjustWindowRectEx(&rect, _style, false, _ex_style);

    _hwnd = ::CreateWindowEx(_ex_style, windows_class_name, title.data(), _style, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, nullptr, nullptr, _hinstance, nullptr);
    ::SetWindowLongPtr(_hwnd, GWLP_USERDATA, LONG_PTR(this));

    y_debug_assert(_hwnd);
}

Window::~Window() {
    ::DestroyWindow(_hwnd);
}

void Window::close() {
    _run = false;
}

bool Window::update() {
    y_profile();

    MSG msg = {};
    while(::PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    return _run;
}

void Window::show() {
    _run = true;
    ::ShowWindow(_hwnd, SW_SHOW);
    focus();
}

void Window::focus() {
    ::BringWindowToTop(_hwnd);
    ::SetForegroundWindow(_hwnd);
    ::SetFocus(_hwnd);
}

bool Window::has_focus() const {
    return _hwnd == ::GetForegroundWindow();
}

bool Window::is_minimized() const {
    // Win API names, WTF
    return ::IsIconic(_hwnd);
}

math::Vec2ui Window::size() const {
    RECT rect = {};
    ::GetClientRect(_hwnd, &rect);
    return math::Vec2ui(u32(std::abs(rect.right - rect.left)), u32(std::abs(rect.bottom - rect.top)));
}

math::Vec2i Window::position() const {
    POINT pos = {};
    ::ClientToScreen(_hwnd, &pos);
    return math::Vec2i(pos.x, pos.y);
}

void Window::set_size(const math::Vec2ui& size) {
    RECT rect = {0, 0, LONG(size.x()), LONG(size.y())};
    ::AdjustWindowRectEx(&rect, _style, false, _ex_style);
    ::SetWindowPos(_hwnd, nullptr, 0, 0, u32(rect.right - rect.left), u32(rect.bottom - rect.top), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
}

void Window::set_position(const math::Vec2i& pos) {
    RECT rect = {pos.x(), pos.y(), 0, 0};
    ::AdjustWindowRectEx(&rect, _style, false, _ex_style);
    set_window_position(math::Vec2i(rect.left, rect.top));
}

math::Vec2i Window::window_position() const {
    RECT rect = {};
    ::GetWindowRect(_hwnd, &rect);
    return math::Vec2i(rect.left, rect.top);
}

void Window::set_window_position(const math::Vec2i &pos) {
    ::SetWindowPos(_hwnd, nullptr, pos.x(), pos.y(), 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
}

void Window::set_title(const core::String& title) {
    ::SetWindowTextA(_hwnd, title.data());
}

void Window::set_cursor_shape(CursorShape shape) {
    ::SetCursor(shape == CursorShape::None ? nullptr : ::LoadCursor(nullptr, to_windows_cursor_icon(shape)));
}

}

#endif


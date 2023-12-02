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

#include <y/utils/log.h>
#include <y/utils/format.h>

#ifdef Y_OS_LINUX

#include <xcb/xcb.h>

namespace yave {

static MouseButton to_mouse_button(xcb_button_t button) {
    switch(button) {
        case 0x1: return MouseButton::LeftButton;
        case 0x3: return MouseButton::RightButton;
    }

    y_fatal("Unknown mouse button");
}

static void dispatch_event(Window* window, const xcb_generic_event_t* event) {
    EventHandler* handler = window->event_handler();
    if(!handler) {
        return;
    }

    switch(event->response_type & ~0x80) {
        case XCB_MOTION_NOTIFY: {
            const xcb_motion_notify_event_t* motion = reinterpret_cast<const xcb_motion_notify_event_t*>(event);
            handler->mouse_moved(math::Vec2i(motion->event_x, motion->event_y));
        } break;

        case XCB_BUTTON_PRESS: {
            const xcb_button_press_event_t* press = reinterpret_cast<const xcb_button_press_event_t*>(event);
            handler->mouse_pressed(math::Vec2i(press->event_x, press->event_y), to_mouse_button(press->detail));
        } break;

        case XCB_BUTTON_RELEASE: {
            const xcb_button_release_event_t* release = reinterpret_cast<const xcb_button_release_event_t*>(event);
            handler->mouse_released(math::Vec2i(release->event_x, release->event_y), to_mouse_button(release->detail));
        } break;
    }
}

Window::Window(const math::Vec2ui& size, const core::String& title, Flags flags) {
    _connection = xcb_connect(nullptr, nullptr);
    _window = xcb_generate_id(_connection);

    const xcb_setup_t* setup = xcb_get_setup(_connection);
    const xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    const xcb_screen_t* screen = iter.data;

    const u32 event_mask =
            XCB_EVENT_MASK_EXPOSURE       | XCB_EVENT_MASK_BUTTON_PRESS   |
            XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW   |
            XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE;

    xcb_create_window(_connection,
                      XCB_COPY_FROM_PARENT,
                      _window,
                      screen->root,
                      0, 0,
                      size.x(), size.y(),
                      10,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT,
                      screen->root_visual,
                      XCB_CW_EVENT_MASK, &event_mask);

    set_title(title);
    xcb_flush(_connection);
}

Window::~Window() {
    xcb_disconnect(_connection);
}

void Window::close() {
    _run = false;
}

bool Window::update() {
    while(xcb_generic_event_t* event = xcb_poll_for_event(_connection)) {
        dispatch_event(this, event);
        std::free(event);
    }
    if(xcb_connection_has_error(_connection)) {
        close();
    }
    return _run;
}

void Window::show() {
    xcb_map_window(_connection, _window);
    xcb_flush(_connection);
}

void Window::focus() {
}

bool Window::has_focus() const {
    return true;
}

bool Window::is_minimized() const {
    return false;
}


static xcb_get_geometry_reply_t window_geometry(xcb_connection_t* connection, u32 window) {
    const xcb_get_geometry_cookie_t cookie = xcb_get_geometry(connection, window);
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(connection, cookie, nullptr);
    if(!reply) {
        return {};
    }
    y_defer(std::free(reply));
    return *reply;
}

static u32 find_parent(xcb_connection_t* connection, u32 window) {
    const xcb_query_tree_cookie_t cookie = xcb_query_tree(connection, window);
    xcb_query_tree_reply_t* tree = xcb_query_tree_reply(connection, cookie, nullptr);
    if(!tree) {
        return window;
    }
    y_defer(std::free(tree));
    return tree->parent;
}

static u32 find_grandparent(xcb_connection_t* connection, u32 window) {
    return find_parent(connection, find_parent(connection, window));
}

math::Vec2ui Window::size() const {
    const xcb_get_geometry_reply_t geom = window_geometry(_connection, _window);
    return math::Vec2i(geom.width, geom.height);
}

math::Vec2i Window::position() const {
    const xcb_get_geometry_reply_t geom = window_geometry(_connection, _window);
    const xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(_connection, _window, find_grandparent(_connection, _window), geom.x, geom.y);
    xcb_translate_coordinates_reply_t* translate = xcb_translate_coordinates_reply(_connection, cookie, nullptr);
    if(!translate) {
        return {};
    }
    y_defer(std::free(translate));
    return math::Vec2i(translate->dst_x, translate->dst_y);
}

void Window::set_size(const math::Vec2ui& size) {
    xcb_configure_window(_connection, _window, XCB_CONFIG_WINDOW_WIDTH| XCB_CONFIG_WINDOW_HEIGHT, size.data());
}

void Window::set_position(const math::Vec2i& pos) {
    const math::Vec2ui upos = pos;
    xcb_configure_window(_connection, _window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, upos.data());
}

math::Vec2i Window::window_position() const {
    const xcb_get_geometry_reply_t geom = window_geometry(_connection, _window);
    const xcb_translate_coordinates_cookie_t cookie = xcb_translate_coordinates(_connection, _window, find_grandparent(_connection, _window), geom.x, geom.y);
    xcb_translate_coordinates_reply_t* translate = xcb_translate_coordinates_reply(_connection, cookie, nullptr);
    if(!translate) {
        return {};
    }
    y_defer(std::free(translate));
    return math::Vec2i(translate->dst_x - geom.x, translate->dst_y - geom.y);
}

void Window::set_window_position(const math::Vec2i& pos) {
    const xcb_get_geometry_reply_t geom = window_geometry(_connection, _window);
    set_position(pos + math::Vec2i(geom.x, geom.y));
}

void Window::set_title(const core::String& title) {
    xcb_change_property(_connection,
        XCB_PROP_MODE_REPLACE,
        _window,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        title.size(),
        title.data());
}

void Window::set_cursor_shape(CursorShape shape) {
}

}

#endif


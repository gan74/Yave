/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <string_view>
#include <memory>


#ifdef Y_OS_WIN
struct HINSTANCE__;
struct HWND__;
#endif

namespace yave {

#ifdef Y_OS_WIN
using HINSTANCE_ = HINSTANCE__*;
using HWND_ = HWND__*;
using LRESULT_ = long long int;
#endif

class Window : NonMovable {
    public:
        enum Flags {
            NoFlags = 0,
            Resizable = 0x01,
            NoDecoration = 0x02,
        };


        Window(const math::Vec2ui& size, std::string_view title, Flags flags = Resizable);
        virtual ~Window();

        void close();
        bool update();

        void show();

        void focus();
        bool has_focus() const;

        bool is_minimized() const;

        math::Vec2ui size() const;
        math::Vec2i position() const;

        void set_size(const math::Vec2ui& size);
        void set_position(const math::Vec2i& pos);

        math::Vec2ui window_size() const;
        math::Vec2i window_position() const;

        void set_window_size(const math::Vec2ui& size);
        void set_window_position(const math::Vec2i& pos);

        void set_title(std::string_view title);


        void set_event_handler(EventHandler* handler) { _event_handler = handler; }
        EventHandler* event_handler() const { return _event_handler; }

#ifdef Y_OS_WIN
        HINSTANCE_ instance() const { return _hinstance; }
        HWND_ handle() const { return _hwnd; }
#endif

    protected:
        virtual void resized() {}

    private:
#ifdef Y_OS_WIN
        friend void notify_resized(Window* win);
        friend void update_capture_state(Window* win, bool button_down, MouseButton button);

        HINSTANCE_ _hinstance = nullptr;
        HWND_ _hwnd = nullptr;
        bool _run = true;
        u32 _mouse_state = 0;

        u32 _style = 0;
        u32 _ex_style = 0;
#endif


        EventHandler* _event_handler = nullptr;
};

}

#endif // YAVE_WINDOW_WINDOW_H


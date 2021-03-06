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

#include <y/core/String.h>

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
            Resizable = 0x01
        };


        Window(const math::Vec2ui& size, std::string_view title, Flags flags = NoFlags);
        virtual ~Window();

        void close();
        bool update();

        void show();

        void set_size(const math::Vec2ui& size);
        void set_position(const math::Vec2ui& pos);

        math::Vec2ui size() const;
        math::Vec2ui position() const;


        void set_title(std::string_view title);


        void set_event_handler(std::unique_ptr<EventHandler> handler) { _event_handler = std::move(handler); }
        EventHandler* event_handler() const { return _event_handler.get(); }

#ifdef Y_OS_WIN
        HINSTANCE_ instance() const { return _hinstance; }
        HWND_ handle() const { return _hwnd; }
#endif

    protected:
        virtual void resized() {}

    private:
#ifdef Y_OS_WIN
        friend void notify_resized(Window* win);

        HINSTANCE_ _hinstance;
        HWND_ _hwnd;
        bool _run;
#endif


        mutable std::unique_ptr<EventHandler> _event_handler;
};

}

#endif // YAVE_WINDOW_WINDOW_H


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
#include "Monitor.h"

#ifdef Y_OS_LINUX

#include <xcb/xcb.h>

namespace yave {

core::Vector<Monitor> Monitor::monitors() {

    xcb_connection_t* connection = xcb_connect(nullptr, nullptr);
    y_defer(xcb_disconnect(connection));

    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

    core::Vector<Monitor> monitors;
    while(iter.rem) {
        const xcb_screen_t* screen = iter.data;

        Monitor monitor = {};
        monitor.is_primary = iter.index == 0;
        monitor.size = {screen->width_in_pixels, screen->height_in_pixels};
        monitor.work_size = monitor.size;

        monitors << monitor;
        xcb_screen_next(&iter);
    }

    return monitors;
}

}

#endif


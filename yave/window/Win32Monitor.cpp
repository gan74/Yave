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

#ifdef Y_OS_WIN

#include <windows.h>
#include <winuser.h>

namespace yave {

core::Vector<Monitor> Monitor::monitors() {
    static_assert(sizeof(LPARAM) >= sizeof(void*));

    const MONITORENUMPROC discover_monitor = [](HMONITOR monitor_handle, HDC, LPRECT, LPARAM monitors_ptr) -> int {
        MONITORINFO info = {};
        info.cbSize = sizeof(MONITORINFO);
        if(!::GetMonitorInfo(monitor_handle, &info)) {
            return false;
        }

        Monitor monitor;
        monitor.position        = math::Vec2i(info.rcMonitor.left, info.rcMonitor.top);
        monitor.work_position   = math::Vec2i(info.rcWork.left, info.rcWork.top);
        monitor.size            = math::Vec2ui(info.rcMonitor.right - info.rcMonitor.left, info.rcMonitor.bottom - info.rcMonitor.top);
        monitor.work_size       = math::Vec2ui(info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top);
        monitor.is_primary      = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

        reinterpret_cast<core::Vector<Monitor>*>(monitors_ptr)->push_back(monitor);

        return true;
    };

    core::Vector<Monitor> monitors;
    y_always_assert(EnumDisplayMonitors(nullptr, nullptr, discover_monitor, reinterpret_cast<LPARAM>(&monitors)), "Unable to enum monitors");

    return monitors;
}

}

#endif


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
#ifndef YAVE_GRAPHICS_COMMANDS_CMDTIMINGRECORDER_H
#define YAVE_GRAPHICS_COMMANDS_CMDTIMINGRECORDER_H

#include "TimeQueryPool.h"

#include <y/core/Chrono.h>
#include <y/core/String.h>

namespace yave {

class CmdTimingRecorder : NonMovable {
    public:
        enum class EventType {
            BeginZone,
            EndZone
        };

        struct Event {
            EventType type;
            core::String name;
            TimestampQuery gpu_ticks;
            u64 cpu_nanos;

            Event(EventType t, const char* n, TimestampQuery&& q, u64 c) : type(t), name(n), gpu_ticks(q), cpu_nanos(c) {
            }
        };

        CmdTimingRecorder(const CmdBufferRecorder& recorder);

        VkCommandBuffer vk_cmd_buffer() const;

        bool is_data_ready() const;

        core::Span<Event> events() const;

    private:
        friend class CmdBufferRegion;

        void begin_zone(const char* name);
        void end_zone();

    private:
        core::Vector<Event> _events;
        TimeQueryPool _query_pool;
        core::Chrono _cpu;
};

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDTIMINGRECORDER_H


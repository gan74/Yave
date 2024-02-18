/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_COMMANDS_TIMELINE_H
#define YAVE_GRAPHICS_COMMANDS_TIMELINE_H

#include <yave/graphics/graphics.h>

#include <atomic>

namespace yave {

class TimelineFence {
    public:
        TimelineFence() = default;

        void wait() const;
        bool is_ready() const;

        u64 value() const;
        bool is_valid() const;

        bool operator==(const TimelineFence& other) const;
        bool operator!=(const TimelineFence& other) const;

        bool operator<(const TimelineFence& other) const;
        bool operator<=(const TimelineFence& other) const;

    private:
        friend class Timeline;

        TimelineFence(u64 value, const Timeline* parent);

        u64 _value = 0;
        const Timeline* _parent = nullptr;
};

class Timeline : NonMovable {
    public:
        Timeline();
        ~Timeline();

        TimelineFence advance_timeline();
        TimelineFence current_timeline() const;

        TimelineFence last_ready() const;
        bool is_ready(TimelineFence fence) const;

        void wait(TimelineFence fence) const;
        void wait() const;

        VkSemaphore vk_semaphore() const;

    private:
        const VkSemaphore _semaphore;

        std::atomic<u64> _value = 0;
        mutable std::atomic<u64> _ready = 0;
};

}

#endif // YAVE_GRAPHICS_COMMANDS_TIMELINE_H


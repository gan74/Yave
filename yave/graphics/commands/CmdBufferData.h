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

#ifndef YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H
#define YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H

#include <yave/graphics/graphics.h>

#include <y/core/Vector.h>

#include <atomic>
#include <memory>

namespace yave {

class ResourceFence {
    public:
        ResourceFence() = default;

        bool operator==(const ResourceFence& other) const;
        bool operator!=(const ResourceFence& other) const;

        bool operator<(const ResourceFence& other) const;
        bool operator<=(const ResourceFence& other) const;

    private:
        friend class LifetimeManager;

        ResourceFence(u64 v);

        u64 _value = 0;
};

class TimelineFence {
    public:
        TimelineFence() = default;
        TimelineFence(u64 v);

        u64 value() const;

        bool operator==(const TimelineFence& other) const;
        bool operator!=(const TimelineFence& other) const;

        bool operator<(const TimelineFence& other) const;
        bool operator<=(const TimelineFence& other) const;

    private:
        friend class CmdQueue;

        u64 _value = u64(-1);
};



class CmdBufferData final : NonMovable {
    public:
        CmdBufferData(VkCommandBuffer buf, CmdBufferPool* p);
        ~CmdBufferData();

        bool is_null() const;

        CmdBufferPool* pool() const;

        ResourceFence resource_fence() const;
        TimelineFence queue_fence() const;

        VkCommandBuffer vk_cmd_buffer() const;

        void wait();
        bool poll();

    private:
        friend class CmdBufferPool;
        friend class CmdQueue;

        void begin();

        // These are owned by the command pool
        VkCommandBuffer _cmd_buffer = {};

        CmdBufferPool* _pool = nullptr;

        ResourceFence _resource_fence;
        TimelineFence _timeline_fence;
};

}

#endif // YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H


/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "Timeline.h"

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

class CmdBufferData final : NonMovable {
    public:
        CmdBufferData(VkCommandBuffer buffer, CmdBufferPool* pool, VkCommandBufferLevel level);
        ~CmdBufferData();

        void push_secondary(CmdBufferData* data);

        bool is_null() const;
        bool is_secondary() const;

        CmdBufferPool* pool() const;
        CmdQueue* queue() const;

        ResourceFence resource_fence() const;
        TimelineFence timeline_fence() const;

        VkCommandBuffer vk_cmd_buffer() const;

        bool is_ready() const;

    private:
        friend class CmdBufferPool;
        friend class CmdQueue;

        void reset();

        // These are owned by the command pool
        const VkCommandBuffer _cmd_buffer;

        VkHandle<VkSemaphore> _semaphore;

        CmdBufferPool* _pool = nullptr;

        ResourceFence _resource_fence;
        TimelineFence _timeline_fence;

        core::SmallVector<CmdBufferData*, 8> _secondaries;
        const VkCommandBufferLevel _level;
};

}

#endif // YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H


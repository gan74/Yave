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
#ifndef YAVE_DEVICE_LIFETIMEMANAGER_H
#define YAVE_DEVICE_LIFETIMEMANAGER_H

#include "DeviceLinked.h"
#include "ResourceType.h"

#include <yave/graphics/descriptors/DescriptorSetAllocator.h>
#include <yave/graphics/commands/CmdBufferData.h>
#include <yave/graphics/memory/DeviceMemory.h>
#include <yave/graphics/vk/vk.h>

#include <variant>
#include <deque>
#include <mutex>

namespace yave {

class LifetimeManager : public DeviceLinked {

    struct EmptyResource {};

    using ManagedResource = std::variant<
#define YAVE_GENERATE_RT_VARIANT(T) T,
YAVE_GRAPHIC_RESOURCE_TYPES(YAVE_GENERATE_RT_VARIANT)
#undef YAVE_GENERATE_RT_VARIANT
        EmptyResource
        >;

    struct InFlightCmdBuffer {
        ResourceFence fence;
        std::unique_ptr<CmdBufferData> cmd_buffer;

        bool operator<(const InFlightCmdBuffer& cmd) const {
            return fence < cmd.fence;
        }
    };

    public:
        LifetimeManager(DevicePtr dptr);
        ~LifetimeManager();

        ResourceFence create_fence();

        void recycle(std::unique_ptr<CmdBufferData> cmd);

        void collect();

        usize pending_deletions() const;
        usize active_cmd_buffers() const;


#define YAVE_GENERATE_DESTROY(T)                                                \
        void destroy_later(T&& t) {                                             \
            const std::unique_lock lock(_resource_lock);                        \
            _to_destroy.emplace_back(_counter, ManagedResource(y_fwd(t)));      \
        }
YAVE_GRAPHIC_RESOURCE_TYPES(YAVE_GENERATE_DESTROY)
#undef YAVE_GENERATE_DESTROY



    private:
        void destroy_resource(ManagedResource& resource) const;
        void clear_resources(u64 up_to);


        std::deque<std::pair<u64, ManagedResource>> _to_destroy;
        std::deque<InFlightCmdBuffer> _in_flight;

        mutable std::mutex _cmd_lock;
        mutable std::mutex _resource_lock;

        std::atomic<u64> _counter = 0;
        u64 _done_counter = 0;
};

}

#endif // YAVE_DEVICE_RESOURCELIFETIMEMANAGER_H


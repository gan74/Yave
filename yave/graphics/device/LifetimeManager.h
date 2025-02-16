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
#ifndef YAVE_DEVICE_LIFETIMEMANAGER_H
#define YAVE_DEVICE_LIFETIMEMANAGER_H

#include <yave/graphics/graphics.h>

#include <yave/graphics/descriptors/DescriptorSetAllocator.h>
#include <yave/graphics/memory/DeviceMemory.h>
#include <yave/material/MaterialDrawData.h>
#include <yave/meshes/MeshDrawData.h>

#include <y/concurrent/Mutexed.h>
#include <y/core/RingQueue.h>

#include <variant>
#include <mutex>
#include <thread>

#define YAVE_MT_LIFETIME_MANAGER

namespace yave {

class LifetimeManager : NonMovable {

    struct EmptyResource {};

    using ManagedResource = std::variant<
#define YAVE_GENERATE_RT_VARIANT(T) T,
YAVE_GRAPHIC_HANDLE_TYPES(YAVE_GENERATE_RT_VARIANT)
#undef YAVE_GENERATE_RT_VARIANT
        EmptyResource
        >;


    public:
        LifetimeManager();
        ~LifetimeManager();

        void shutdown_collector_thread();

        ResourceFence create_fence();
        void register_pending(core::Span<CmdBufferData*> datas);

        usize pending_deletions() const;
        usize pending_cmd_buffers() const;

        void collect_cmd_buffers();
        void wait_cmd_buffers();

#define YAVE_GENERATE_DESTROY(T)                                                        \
        void destroy_later(T&& t) {                                                     \
            _to_destroy.locked([&](auto& to_destroy) {                                  \
                to_destroy.emplace_back(_create_counter, ManagedResource(y_fwd(t)));    \
            });                                                                         \
        }
YAVE_GRAPHIC_HANDLE_TYPES(YAVE_GENERATE_DESTROY)
#undef YAVE_GENERATE_DESTROY

    private:
        void clear_resources(u64 up_to);
        void destroy_resource(ManagedResource& resource) const;

        concurrent::Mutexed<core::RingQueue<std::pair<u64, ManagedResource>>> _to_destroy;
        concurrent::Mutexed<core::RingQueue<CmdBufferData*>, std::recursive_mutex> _in_flight;

        std::atomic<u64> _create_counter = 0;
        u64 _next_to_collect = 0; // Guarded by _in_flight

#ifdef YAVE_MT_LIFETIME_MANAGER
        std::thread _collector_thread;
        std::atomic<bool> _run_thread = true;
#endif

};

}

#endif // YAVE_DEVICE_RESOURCELIFETIMEMANAGER_H

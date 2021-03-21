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

#ifndef YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H
#define YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H

#include <yave/graphics/graphics.h>

#include <y/core/Vector.h>

#include <memory>
#include <atomic>

namespace yave {

class ResourceFence {
    public:
        ResourceFence() = default;

        bool operator==(const ResourceFence& other) const {
            return _value == other._value;
        }

        bool operator!=(const ResourceFence& other) const {
            return _value != other._value;
        }


        bool operator<(const ResourceFence& other) const {
            return _value < other._value;
        }

        bool operator<=(const ResourceFence& other) const {
            return _value <= other._value;
        }


        bool operator>(const ResourceFence& other) const {
            return _value > other._value;
        }

        bool operator>=(const ResourceFence& other) const {
            return _value >= other._value;
        }

    private:
        friend class LifetimeManager;

        ResourceFence(u64 v) : _value(v) {
        }

        u64 _value = 0;
};



class CmdBufferData final : NonMovable {
    struct KeepAlive : NonCopyable {
        virtual ~KeepAlive() {}
    };

    public:
        CmdBufferData(VkCommandBuffer buf, VkFence fen, CmdBufferPool* p);
        ~CmdBufferData();

        bool is_null() const;

        bool is_signaled() const;

        CmdBufferPool* pool() const;
        ResourceFence resource_fence() const;

        VkCommandBuffer vk_cmd_buffer() const;
        VkFence vk_fence() const;

        void wait();
        bool poll_and_signal();

        template<typename T>
        void keep_alive(T&& t) {
            struct Box : KeepAlive {
                Box(T&& t) : _t(y_fwd(t)) {}
                ~Box() override {}
                std::remove_reference_t<T> _t;
            };
            _keep_alive.emplace_back(std::make_unique<Box>(y_fwd(t)));
        }

    private:
        friend class CmdBufferPool;

        void set_signaled();

        void begin();
        void recycle_resources(); // This can not be called while pool's _pending_lock is locked


        // These are owned by the command pool
        VkCommandBuffer _cmd_buffer;
        VkFence _fence;

        core::Vector<std::unique_ptr<KeepAlive>> _keep_alive;
        CmdBufferPool* _pool = nullptr;

        ResourceFence _resource_fence;

        std::atomic<bool> _signaled = false;

#ifdef Y_DEBUG
        std::atomic<bool> _lock = false;
#endif

};

}

#endif // YAVE_GRAPHICS_COMMANDS_DATA_CMDBUFFERDATA_H


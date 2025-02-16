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

#ifndef Y_CONCURRENT_SIGNAL_H
#define Y_CONCURRENT_SIGNAL_H

#include "Mutexed.h"

#include <y/core/Vector.h>

#include <memory>
#include <functional>

namespace y {
namespace concurrent {
namespace detail {
struct SignalDataBase {
    virtual ~SignalDataBase() {
    }

    virtual void disconnect(u32 index) = 0;
};
}


class [[nodiscard]] Subscription : NonCopyable {
    public:
        Subscription() = default;
        ~Subscription();

        Subscription(Subscription&& other);
        Subscription& operator=(Subscription&& other);

        bool is_connected() const;

        void disconnect();
        void detach() &&;

        void swap(Subscription& other);

    private:
        template<typename... Args>
        friend class Signal;

        Subscription(std::weak_ptr<detail::SignalDataBase> data, u32 index);

        std::weak_ptr<detail::SignalDataBase> _data;
        u32 _index = 0;
};



template<typename... Args>
class Signal {
    struct Data : detail::SignalDataBase {
        struct Slot {
            std::function<void(Args...)> func;
            u32 index;
        };

        Mutexed<core::Vector<Slot>, std::shared_mutex> _receivers;
        std::atomic<u32> counter = 0;

        void disconnect(u32 index) override {
            _receivers.locked([&](auto&& receivers) {
                auto it = std::find_if(receivers.begin(), receivers.end(), [&](const auto& rec) {
                    return rec.index == index;
                });
                y_debug_assert(it != receivers.end());
                receivers.erase_unordered(it);
            });
        }
    };

    public:
        Signal() : _data(std::make_shared<Data>()) {
        }

        template<typename F>
        Subscription subscribe(F&& func) {
            const u32 index = ++_data->counter;
            _data->_receivers.locked([&](auto&& receivers) {
                receivers.emplace_back(y_fwd(func), index);
            });
            return Subscription(_data, index);
        }

        void send(Args... args) const {
            _data->_receivers.locked_shared([&](const auto& receivers) {
                for(const auto& rec : receivers) {
                    rec.func(args...);
                }
            });
        }

    private:
        friend class Subscription;

        std::shared_ptr<Data> _data;
};

}
}

#endif // Y_CONCURRENT_SIGNAL_H


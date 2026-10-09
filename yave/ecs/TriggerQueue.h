/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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
#ifndef YAVE_ECS_TRIGGERQUEUE_H
#define YAVE_ECS_TRIGGERQUEUE_H

#include "SparseComponentSet.h"

#include <y/concurrent/Mutexed.h>
#include <y/concurrent/SpinLock.h>

#include <concepts>
#include <functional>

namespace yave {
namespace ecs {

enum class TriggerTypeIndex : u32 {
    invalid_index = u32(-1),
};

namespace detail {
TriggerTypeIndex next_trigger_index();

template<typename T>
inline const TriggerTypeIndex trigger_index_v = next_trigger_index();
}

usize registered_trigger_type_count();

template<typename T>
TriggerTypeIndex trigger_index() {
    static_assert(!std::is_const_v<T> && !std::is_reference_v<T>);
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);
    return detail::trigger_index_v<T>;
}



class TriggerHandler : NonMovable {
    public:
        virtual ~TriggerHandler() = default;

        virtual void on_trigger(EntityWorld& world, EntityId target, TriggerTypeIndex type, const void* payload) = 0;
};

template<typename T>
class TriggerCallback final : public TriggerHandler {
    public:
        template<std::invocable<EntityWorld&, EntityId, const T&> F>
        TriggerCallback(F&& func) : _func(y_fwd(func)) {
        }

        void on_trigger(EntityWorld& world, EntityId target, TriggerTypeIndex type, const void* payload) override {
            y_debug_assert(type == trigger_index<T>());
            unused(type);
            _func(world, target, *static_cast<const T*>(payload));
        }

    private:
        std::function<void(EntityWorld&, EntityId, const T&)> _func;
};




class TriggerQueueBase : NonMovable {
    public:
        virtual ~TriggerQueueBase() = default;

        virtual void take_pending() = 0;

        virtual void dispatch(EntityWorld& world) = 0;

        TriggerTypeIndex type() const;

        bool is_listened(EntityId target) const;

        bool has_handlers() const;

        core::Span<ecs::EntityId> listened_ids() const;

        void subscribe(EntityId target, TriggerHandler* handler);
        void unsubscribe(EntityId target, TriggerHandler* handler);

    protected:
        TriggerQueueBase(TriggerTypeIndex type);

        void dispatch_one(EntityWorld& world, EntityId target, const void* payload) const;

    private:
        TriggerTypeIndex _type;

        core::Vector<TriggerHandler*> _global_handlers;
        SparseComponentSet<core::SmallVector<TriggerHandler*, 4>> _handlers;
};

template<typename T>
class TriggerQueue final : public TriggerQueueBase {
    public:
        struct Event {
            EntityId target;
            T payload;
        };

        TriggerQueue() : TriggerQueueBase(trigger_index<T>()) {
        }

        void push(EntityId target, const T& payload) {
            _pending.locked([&](auto&& pending) { pending << Event{target, payload}; });
        }

        void take_pending() override {
            y_debug_assert(_ready.is_empty());
            _pending.locked([&](auto&& pending) { _ready.swap(pending); });
        }

        void dispatch(EntityWorld& world) override {
            for(const Event& event : _ready) {
                dispatch_one(world, event.target, &event.payload);
            }
            _ready.make_empty();
        }

    private:
        concurrent::Mutexed<core::Vector<Event>, concurrent::SpinLock> _pending;
        core::Vector<Event> _ready;
};

}
}

#endif // YAVE_ECS_TRIGGERQUEUE_H


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

#include "TriggerQueue.h"
#include "EntityWorld.h"

#include <atomic>
#include <algorithm>

namespace yave {
namespace ecs {
static std::atomic<std::underlying_type_t<TriggerTypeIndex>>& global_trigger_index() {
    static std::atomic<std::underlying_type_t<TriggerTypeIndex>> index = 0;
    return index;
}

namespace detail {
TriggerTypeIndex next_trigger_index() {
    return TriggerTypeIndex(global_trigger_index()++);
}
}



usize registered_trigger_type_count() {
    return usize(global_trigger_index().load());
}





TriggerQueueBase::TriggerQueueBase(TriggerTypeIndex type) : _type(type) {
}

TriggerTypeIndex TriggerQueueBase::type() const {
    return _type;
}

bool TriggerQueueBase::is_listened(EntityId target) const {
    return !_global_handlers.is_empty() || _handlers.contains(target);
}

bool TriggerQueueBase::has_handlers() const {
    return !_global_handlers.is_empty() || !_handlers.is_empty();
}

void TriggerQueueBase::subscribe(EntityId target, TriggerHandler* handler) {
    y_debug_assert(handler);
    if(target.is_valid()) {
        _handlers.get_or_insert(target) << handler;
    } else {
        _global_handlers << handler;
    }
}

void TriggerQueueBase::unsubscribe(EntityId target, TriggerHandler* handler) {
    const auto remove = [=](auto& handlers) {
        if(const auto it = std::find(handlers.begin(), handlers.end(), handler); it != handlers.end()) {
            handlers.erase_unordered(it);
        }
    };

    if(target.is_valid()) {
        if(auto* handlers = _handlers.try_get(target)) {
            remove(*handlers);
            if(handlers->is_empty()) {
                _handlers.erase(target);
            }
        }
    } else {
        remove(_global_handlers);
    }
}

void TriggerQueueBase::dispatch_one(EntityWorld& world, EntityId target, const void* payload) const {
    if(target.is_valid() && !world.exists(target)) {
        return;
    }

    for(TriggerHandler* handler : _global_handlers) {
        handler->on_trigger(world, target, _type, payload);
    }

    if(const auto* handlers = _handlers.try_get(target)) {
        for(TriggerHandler* handler : *handlers) {
            handler->on_trigger(world, target, _type, payload);
        }
    }
}

}
}


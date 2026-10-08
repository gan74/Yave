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

#include "TriggerManager.h"
#include "EntityWorld.h"

namespace yave {
namespace ecs {

TriggerManager::TriggerManager() : _queues(registered_trigger_type_count()) {
}

void TriggerManager::reset() {
    std::fill_n(_queues.data(), _queues.size(), nullptr);
}

bool TriggerManager::is_listened(TriggerTypeIndex type, EntityId target) const {
    const TriggerQueueBase* queue = _queues[usize(type)].get();
    return queue && queue->is_listened(target);
}


void TriggerManager::unsubscribe(TriggerTypeIndex type, EntityId target, TriggerHandler* handler) {
    y_debug_assert(!_dispatching);
    if(const auto& queue = find_queue(type)) {
        queue->unsubscribe(target, handler);
    }
}

void TriggerManager::dispatch(EntityWorld& world) {
    y_profile();

    for(auto& queue : _queues) {
        if(queue) {
            queue->take_pending();
        }
    }

    y_debug_assert(!_dispatching);
    
#ifdef Y_DEBUG
    _dispatching = true;
    y_defer(_dispatching = false);
#endif

    for(auto& queue : _queues) {
        if(queue) {
            queue->dispatch(world);
        }
    }
}

TriggerQueueBase* TriggerManager::find_queue(TriggerTypeIndex type) {
    return _queues[usize(type)].get();
}

}
}


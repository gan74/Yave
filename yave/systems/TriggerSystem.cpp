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

#include "TriggerSystem.h"

#include <yave/ecs/SystemManager.h>

namespace yave {


void TriggerSystem::ComponentSubscriptionsBase::unsubscribe_all(ecs::TriggerManager& triggers) {
    for(const ecs::EntityId id : _subscribed.ids()) {
        for(const ecs::TriggerTypeIndex type : _subscribed[id]) {
            triggers.unsubscribe(type, id, this);
        }
    }
    _subscribed.make_empty();
    _dirty.make_empty();
}

void TriggerSystem::ComponentSubscriptionsBase::unsubscribe(ecs::TriggerManager& triggers, ecs::EntityId id) {
    if(const auto* types = _subscribed.try_get(id)) {
        for(const ecs::TriggerTypeIndex type : *types) {
            triggers.unsubscribe(type, id, this);
        }
        _subscribed.erase(id);
    }
}



TriggerSystem::TriggerSystem() : ecs::System("TriggerSystem") {
}

void TriggerSystem::setup(ecs::SystemScheduler& sched) {
    for(const auto& components : _components) {
        components->collect_all(world());
        components->setup(sched);
    }

    sched.schedule(ecs::SystemSchedule::TickSequential, "Dispatch triggers", [this]() {
        for(const auto& components : _components) {
            components->apply(world());
        }
        world().triggers().dispatch(world());
    });
}

void TriggerSystem::reset() {
    for(const auto& components : _components) {
        components->unsubscribe_all(world().triggers());
    }
}

}


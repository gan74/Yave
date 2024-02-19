/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "TransformableManagerSystem.h"

#include <yave/components/TransformableComponent.h>

#include <yave/ecs/EntityWorld.h>

namespace yave {

TransformableManagerSystem::TransformableManagerSystem() : ecs::System("TransformableManagerSystem") {
}

void TransformableManagerSystem::destroy() {
    _transform_destroyed.disconnect();
    auto query = world().query<TransformableComponent>();
    for(const auto& [tr] : query.components()) {
        free_index(tr);
    }
}

void TransformableManagerSystem::setup() {
    _transform_destroyed = world().on_destroyed<TransformableComponent>().subscribe([this](ecs::EntityId, TransformableComponent& tr) {
        free_index(tr);
    });

    run_tick(false);
}

void TransformableManagerSystem::tick() {
    run_tick(true);
}

void TransformableManagerSystem::run_tick(bool only_recent) {
    y_profile();

    auto moved_query = only_recent ? world().query<ecs::Changed<TransformableComponent>>() : world().query<TransformableComponent>();

    _stopped.swap(_moved);
    _moved.make_empty();

    for(const auto& [id, comp] : moved_query.id_components()) {
        const auto& [tr] = comp;
        _moved.insert(id, alloc_index(tr));
        _stopped.erase(id);
    }
}


void TransformableManagerSystem::free_index(const TransformableComponent& tr) {
    if(tr._transform_index != u32(-1)) {
        _free.push_back(tr._transform_index);
        tr._transform_index = u32(-1);
    }
}

TransformableManagerSystem::TransformIndex TransformableManagerSystem::alloc_index(const TransformableComponent& tr) {
    if(tr._transform_index == u32(-1)) {
        if(!_free.is_empty()) {
            tr._transform_index = _free.pop();
        } else {
            tr._transform_index = _max_index++;
        }

        return TransformIndex {
            tr._transform_index, true,
        };
    }

    return TransformIndex {
        tr._transform_index, false,
    };
}


u32 TransformableManagerSystem::transform_count() const {
    return _max_index;
}




}


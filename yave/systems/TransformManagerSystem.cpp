/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "TransformManagerSystem.h"

#include <yave/components/TransformableComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

TransformManagerSystem::TransformManagerSystem() : ecs::System("TransformManagerSystem") {
}

void TransformManagerSystem::setup() {
    {
        u32 max_index = 0;
        const auto& set = world().component_set<TransformableComponent>();
        for(const auto& [id, tr] : set.as_pairs()) {
            tr._manager = this;
            tr._manager_index = id.index();
            max_index = std::max(max_index, id.index());
        }
        _transforms.set_min_size(max_index + 1);
    }

    _entity_created = world().on_entity_created().subscribe([this](ecs::EntityId id) {
        _transforms.set_min_size(id.index() + 1);
    });

    _transform_created = world().on_component_created<TransformableComponent>().subscribe([this](ecs::EntityId id, TransformableComponent& tr) {
        tr._manager = this;
        tr._manager_index = id.index();
    });
}


void TransformManagerSystem::set_world_transform(TransformableComponent& tr, const math::Transform<>& world_transform) {
    y_debug_assert(tr._manager_index != u32(-1));
    y_debug_assert(tr._manager == this);

    _transforms[tr._manager_index].world_transform = world_transform;


    const ecs::EntityId id = world().id_from_index(tr._manager_index);
    if(const ecs::EntityId parent = world().parent(id); parent.is_valid()) {
        tr._local_transform = _transforms[parent.index()].world_transform.inverse() * world_transform;
    }

    update_world_transform(tr._manager_index, &tr);
}

const math::Transform<>& TransformManagerSystem::world_transform(const TransformableComponent& tr) const {
    y_debug_assert(tr._manager_index != u32(-1));
    y_debug_assert(tr._manager == this);
    y_debug_assert(tr._manager_index < _transforms.size());

    return _transforms[tr._manager_index].world_transform;
}



void TransformManagerSystem::update_world_transform(u32 index, const TransformableComponent* tr) {
    y_profile();

    const ecs::EntityId id = world().id_from_index(index);

    math::Transform<> parent_transform;
    if(const ecs::EntityId parent = world().parent(id); parent.is_valid()) {
        parent_transform = _transforms[parent.index()].world_transform;
    }

    const math::Transform<> local_transform = tr ? tr->_local_transform : math::Transform<>();
    _transforms[index].world_transform = parent_transform * local_transform;

    make_modified(index);

    for(const ecs::EntityId child : world().children(id)) {
        update_world_transform(child.index(), world().component<TransformableComponent>(child));
    }
}


void TransformManagerSystem::make_modified(u32 index) {
    TransformData& data = _transforms[index];
    if(!data.modified) {
        data.modified = true;
        _modified << index;
    }

}

}


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

#include "EntityWorld.h"

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/utils/memory.h>

#include <yave/assets/AssetLoadingContext.h>

#include <numeric>


namespace yave {
namespace ecs2 {

static auto create_component_containers() {
    y_profile();

    core::Vector<std::unique_ptr<ComponentContainerBase>> containers;
    for(const auto* poly_base = ComponentContainerBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
        if(poly_base->create) {
            std::unique_ptr<ComponentContainerBase> container = poly_base->create();
            y_debug_assert(container);

            const ComponentTypeIndex id = container->type_id();
            containers.set_min_size(usize(id) + 1);
            containers[usize(id)] = std::move(container);
        }
    }
    return containers;
}


EntityWorld::EntityWorld() : _containers(create_component_containers()), _matrix(_containers.size()) {
    for(auto& container : _containers) {
        if(container) {
            container->_matrix = &_matrix;
        }
    }
}

EntityWorld::~EntityWorld() {
    _containers.clear();
}

usize EntityWorld::entity_count() const {
    return _entities.size();
}

bool EntityWorld::exists(EntityId id) const {
    return _entities.exists(id);
}

EntityId EntityWorld::create_entity() {
    const EntityId id = _entities.create();
    _matrix.add_entity(id);
    return id;
}

const ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) const {
    y_debug_assert(_containers.size() > usize(type_id));
    return _containers[usize(type_id)].get();
}

ComponentContainerBase* EntityWorld::find_container(ComponentTypeIndex type_id) {
    y_debug_assert(_containers.size() > usize(type_id));
    return _containers[usize(type_id)].get();
}

void EntityWorld::check_exists(EntityId id) const {
    y_always_assert(exists(id), "Entity doesn't exists");
}

}
}


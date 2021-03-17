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
#ifndef YAVE_ECS_ENTITYWORLD_INL
#define YAVE_ECS_ENTITYWORLD_INL

#ifndef YAVE_ECS_ENTITYWORLD_H
#error this file should not be included directly

// Just to help the IDE
#include "EntityWorld.h"
#endif


namespace yave {
namespace ecs {

template<typename T>
std::unique_ptr<ComponentContainerBase> create_container() {
    return std::make_unique<ComponentContainer<T>>();
}

template<typename T>
void create_component(EntityWorld& world, EntityId id) {
    world.add_component<T>(id);
}


template<typename... Args>
void RequiredComponents<Args...>::add_required_components(EntityWorld& world, EntityId id) {
    world.add_components<Args...>(id);
}




template<typename T>
void ComponentContainerBase::add_required_components(EntityWorld& world, EntityId id) {
    unused(id);
    if constexpr(is_detected_v<detail::has_required_components_t, T>) {
        T::add_required_components(world, id);
    }
}




template<typename T>
ComponentBox<T>::ComponentBox(T t) : _component(std::move(t)) {
}

template<typename T>
ComponentRuntimeInfo ComponentBox<T>::runtime_info() const {
    return ComponentRuntimeInfo::create<T>();
}

template<typename T>
void ComponentBox<T>::add_to(EntityWorld& world, EntityId id) const {
    world.add_component<T>(id, _component);
}

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H


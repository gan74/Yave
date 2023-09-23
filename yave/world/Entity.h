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
#ifndef YAVE_WORLD_ENTITY_H
#define YAVE_WORLD_ENTITY_H

#include "EntityId.h"
#include "ComponentRef.h"

#include <y/core/Vector.h>

namespace yave {

struct Entity : NonCopyable {
    public:
        struct Component {
            ComponentTypeIndex type_index;
            UntypedComponentRef component;
        };


        EntityId id() const {
            return _id;
        }

        UntypedComponentRef get(ComponentType type) const {
            const ComponentTypeIndex index = type.index();
            const auto it = find_type_it(index);
            if(it == _components.end() || it->type_index != index) {
                return {};
            }

            return it->component;
        }

        template<typename T>
        ComponentRef<T> get() const {
            return get(component_type<T>()).to_typed<T>();
        }


    private:
        friend class EntityContainer;

        void register_component(UntypedComponentRef ref) {
            const ComponentTypeIndex index = ref.type().index();
            const auto it = find_type_it(index);

            y_always_assert(it == _components.end() || it->type_index != index, "Component already exist");
            _components.insert(it, Component(index, ref));
        }

        Entity(EntityId id) : _id(id) {
        }




        inline auto find_type_it(ComponentTypeIndex index) const {
            return std::lower_bound(_components.begin(), _components.end(), index, [](const Component& comp, ComponentTypeIndex index) {
                return index < comp.type_index;
            });
        }


        EntityId _id;
        EntityId _parent;
        core::Vector<Component> _components;
};

class EntityContainer {
    public:
        Entity& create_entity() {
            return _entities.push_back(create_id());
        }

        bool exists(EntityId id) const {
            return id._index < _entities.size() && _entities[id._index]._id == id;
        }

        void register_component(EntityId id, UntypedComponentRef ref) {
            y_debug_assert(exists(id));
            return _entities[id._index].register_component(ref);
        }

        const Entity& operator[](EntityId id) const {
            y_debug_assert(exists(id));
            return _entities[id._index];
        }

    private:
        EntityId create_id() {
            EntityId id;
            if(!_free.is_empty()) {
                id = _free.pop();
            } else {
                id._index = u32(_entities.size());
            }

            ++id._generation;
            return id;
        }


        core::Vector<Entity> _entities;
        core::Vector<EntityId> _free;
};

}


#endif // YAVE_WORLD_ENTITY_H


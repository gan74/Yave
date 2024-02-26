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
#ifndef YAVE_ECS2_ENTITYWORLD_H
#define YAVE_ECS2_ENTITYWORLD_H


#include "ComponentContainer.h"
#include "EntityGroup.h"
#include "traits.h"

#include <yave/ecs/EntityPool.h>

namespace yave {
namespace ecs2 {

using EntityPool = ecs::EntityPool;

class EntityWorld : NonMovable {

    public:
        EntityWorld();
        ~EntityWorld();

        usize entity_count() const;
        bool exists(EntityId id) const;

        EntityId create_entity();

        template<typename T>
        T* get_or_add_component(EntityId id) {
            return find_container<T>()->get_or_add(id);
        }

        template<typename T, typename... Args>
        T* add_or_replace_component(EntityId id, Args&&... args) {
            check_exists(id);
            return find_container<T>()->add_or_replace(id, y_fwd(args)...);
        }

        template<typename... Ts>
        const EntityGroup<Ts...>& create_group() {
            using group_type = EntityGroup<Ts...>;
            for(const auto& group : _groups) {
                if(const auto* typed_group = dynamic_cast<group_type*>(group.get())) {
                    return *typed_group;
                }
            }
            std::unique_ptr group = std::make_unique<group_type>();
            group_type* group_ptr = group.get();
            _groups.emplace_back(std::move(group));
            _matrix.register_group(group_ptr);
            return *group_ptr;
        }


    private:
        const ComponentContainerBase* find_container(ComponentTypeIndex type_id) const;
        ComponentContainerBase* find_container(ComponentTypeIndex type_id);

        void check_exists(EntityId id) const;


        core::Vector<std::unique_ptr<ComponentContainerBase>> _containers;
        core::Vector<std::unique_ptr<EntityGroupBase>> _groups;
        ComponentMatrix _matrix;
        EntityPool _entities;


    private:
        template<typename T>
        friend class ComponentContainer;

        template<typename T>
        const ComponentContainer<T>* find_container() const {
            static_assert(std::is_same_v<traits::component_raw_type_t<T>, T>);
            static const auto static_info = ComponentRuntimeInfo::create<T>();
            unused(static_info);
            return static_cast<const ComponentContainer<T>*>(find_container(type_index<T>()));
        }

        template<typename T>
        ComponentContainer<T>* find_container() {
            static_assert(std::is_same_v<traits::component_raw_type_t<T>, T>);
            static const auto static_info = ComponentRuntimeInfo::create<T>();
            unused(static_info);
            return static_cast<ComponentContainer<T>*>(find_container(type_index<T>()));
        }
};

}
}

#include "EntityWorld.inl"

#endif // YAVE_ECS2_ENTITYWORLD_H


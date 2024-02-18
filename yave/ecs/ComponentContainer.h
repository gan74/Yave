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
#ifndef YAVE_ECS_COMPONENTCONTAINER_H
#define YAVE_ECS_COMPONENTCONTAINER_H

#include "ecs.h"
#include "SparseComponentSet.h"
#include "ComponentInspector.h"
#include "ComponentBox.h"

#include <y/concurrent/Signal.h>


namespace yave {
namespace ecs {

namespace detail {
template<typename T>
using has_register_component_type_t = decltype(std::declval<T>().register_component_type(std::declval<System*>()));
template<typename T>
using has_inspect_t = decltype(std::declval<T>().inspect(std::declval<ComponentInspector*>()));
}




class ComponentContainerBase : NonMovable {
    public:
        virtual ~ComponentContainerBase();

        virtual void register_component_type(System* system) const  = 0;

        virtual ComponentRuntimeInfo runtime_info() const = 0;

        virtual std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const = 0;


        virtual void add_if_absent(EntityId id) = 0;
        virtual void remove(EntityId id) = 0;


        virtual void inspect_component(EntityId id, ComponentInspector* inspector) = 0;



        bool contains(EntityId id) const;

        ComponentTypeIndex type_id() const;

        const SparseIdSetBase& id_set() const;
        const SparseIdSet& recently_mutated() const;





        y_serde3_poly_abstract_base(ComponentContainerBase)

    protected:
        friend class EntityWorld;

        ComponentContainerBase(ComponentTypeIndex type_id) : _type_id(type_id) {
        }


        virtual serde3::Result save_state(serde3::WritableArchive& arc) const = 0;
        virtual serde3::Result load_state(serde3::ReadableArchive& arc) = 0;
        virtual void post_load() = 0;


        const ComponentTypeIndex _type_id;
        SparseIdSet _mutated;
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
    public:
        using component_type = T;

        ComponentContainer() : ComponentContainerBase(type_index<T>()) {
        }

        void register_component_type(System* system) const override {
            unused(system);
            if constexpr(is_detected_v<detail::has_register_component_type_t, T>) {
                T::register_component_type(system);
            }
        }

        ComponentRuntimeInfo runtime_info() const override {
            return ComponentRuntimeInfo::create<T>();
        }

        std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const override {
            unused(id);
            if constexpr(std::is_copy_constructible_v<T>) {
                return std::make_unique<ComponentBox<T>>(_components[id]);
            } else {
                return nullptr;
            }
        }

        void inspect_component(EntityId id, ComponentInspector* inspector) override {
            if constexpr(is_detected_v<detail::has_inspect_t, T>) {
                if(T* comp = component_ptr_mut(id)) {
                    if(inspector->inspect_component_type(runtime_info(), true)) {
                        comp->inspect(inspector);
                    }
                }
            } else {
                if(contains(id)) {
                    inspector->inspect_component_type(runtime_info(), false);
                }
            }
        }


        void remove(EntityId id) override {
            if(T* comp = _components.try_get(id)) {
                _on_destroyed.send(id, *comp);
                _components.erase(id);
                _mutated.erase(id);
            }
        }



        void add_if_absent(EntityId id) override {
            get_or_add(id);
        }

        template<typename... Args>
        inline T& add_or_replace(EntityId id, Args&&... args) {
            y_debug_assert(id.is_valid());

            _mutated.insert(id);

            T* comp = nullptr;
            if(!_components.contains_index(id.index())) {
                comp = &_components.insert(id, y_fwd(args)...);
            } else {
                comp = &(_components[id] = std::move(T(y_fwd(args)...)));
            }

            _on_created.send(id, *comp);
            return *comp;
        }

        inline T& get_or_add(EntityId id) {
            y_debug_assert(id.is_valid());

            _mutated.insert(id);

            if(!_components.contains_index(id.index())) {
                T& comp = _components.insert(id);
                _on_created.send(id, comp);
                return comp;
            } else {
                return _components[id];
            }
        }


        inline const T* component_ptr(EntityId id) const {
            return _components.try_get(id);
        }

        inline T* component_ptr_mut(EntityId id) {
            auto* ptr = _components.try_get(id);
            if(ptr) {
                _mutated.insert(id);
            }
            return ptr;
        }



        const SparseComponentSet<T>& component_set() const {
            return _components;
        }

        SparseComponentSet<T>& component_set() {
            return _components;
        }


        y_serde3_poly(ComponentContainer)
        y_reflect(ComponentContainer, _components)

    private:
        friend class EntityWorld;


        serde3::Result save_state(serde3::WritableArchive& arc) const override {
            return arc.serialize(_components);
        }

        serde3::Result load_state(serde3::ReadableArchive& arc) override {
            return arc.deserialize(_components);
        }

        void post_load() override {
            for(auto&& [id, comp] : _components) {
                _mutated.insert(id);
                _on_created.send(id, comp);
            }
        }


        SparseComponentSet<T> _components;

        concurrent::Signal<EntityId, T&> _on_created;
        concurrent::Signal<EntityId, T&> _on_destroyed;
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H


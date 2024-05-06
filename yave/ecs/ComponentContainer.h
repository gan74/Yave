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

#include "ComponentRuntimeInfo.h"
#include "ComponentMatrix.h"
#include "ComponentInspector.h"

#include <y/serde3/poly.h>

namespace yave {
namespace ecs {

class ComponentContainerBase : NonMovable {
    public:
        virtual ~ComponentContainerBase();

        ComponentTypeIndex type_id() const;


        virtual ComponentRuntimeInfo runtime_info() const = 0;

        virtual void remove_later(EntityId id) = 0;

        virtual void inspect_component(EntityId id, ComponentInspector* inspector) = 0;


        y_serde3_poly_abstract_base(ComponentContainerBase)

    protected:
        friend class EntityWorld;

        template<typename... Ts>
        friend class EntityGroup;

        ComponentContainerBase(ComponentTypeIndex type_id) : _type_id(type_id) {
        }

        virtual void register_component_type(System* system) const = 0;
        virtual void post_load() = 0;
        virtual void process_deletions() = 0;


        const ComponentTypeIndex _type_id;
        ComponentMatrix* _matrix = nullptr;
        SparseIdSet _mutated;
        SparseIdSet _to_delete;

        std::shared_mutex _lock;
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
    public:
        using component_type = T;

        ComponentContainer() : ComponentContainerBase(type_index<T>()) {
        }

        const SparseComponentSet<T>& component_set() const {
            return _components;
        }


        const T* try_get(EntityId id) const {
            return _components.try_get(id);
        }

        T* try_get_mut(EntityId id) {
            T* comp = _components.try_get(id);
            if(comp) {
                _mutated.insert(id);
            }
            return comp;
        }

        T* get_or_add(EntityId id) {
            if(_components.contains(id)) {
                return &_components[id];
            }
            return add(id);
        }

        template<typename... Args>
        T* add_or_replace(EntityId id, Args&... args) {
            if(_components.contains(id)) {
                return &(_components[id] = T(y_fwd(args)...));
            }
            return add(id, y_fwd(args)...);
        }



        ComponentRuntimeInfo runtime_info() const override {
            return ComponentRuntimeInfo::create<T>();
        }

        void remove_later(EntityId id) override {
            if(_components.contains(id)) {
                _to_delete.insert(id);
            }
        }

        void inspect_component(EntityId id, ComponentInspector* inspector) override {
            if(!_components.contains(id)) {
                return;
            }

            if constexpr(Inspectable<T>) {
                if(inspector->inspect_component_type(runtime_info(), true)) {
                    _components.try_get(id)->inspect(inspector);
                    _mutated.insert(id);
                }
            } else {
                if(inspector->inspect_component_type(runtime_info(), false)) {
                    _mutated.insert(id);
                }
            }
        }


        y_serde3_poly(ComponentContainer)
        y_reflect(ComponentContainer, _components)

    private:
        friend class EntityWorld;

        template<typename... Ts>
        friend class EntityGroup;


        void register_component_type(System* system) const override {
            unused(system);
            if constexpr(Registerable<T>) {
                T::register_component_type(system);
            }
        }

        void post_load() override {
            for(auto&& [id, comp] : _components) {
                _matrix->add_component<T>(id);
                _mutated.insert(id);
            }
        }

        void process_deletions() override {
            for(const EntityId id : _to_delete) {
                _matrix->remove_component<T>(id);
                _components.erase(id);
                _mutated.erase(id);
            }
            _to_delete.make_empty();
        }



        template<typename... Args>
        T* add(EntityId id, Args&... args) {
            T* component =  &_components.insert(id, y_fwd(args)...);
            _matrix->add_component<T>(id);
            return component;
        }



        SparseComponentSet<T> _components;
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H


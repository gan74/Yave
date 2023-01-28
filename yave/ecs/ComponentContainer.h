/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include "traits.h"
#include "SparseComponentSet.h"
#include "ComponentRuntimeInfo.h"

Y_TODO(try replacing this?)
#include <y/serde3/archives.h>

namespace yave {
namespace ecs {

namespace detail {
template<typename T>
using has_required_components_t = decltype(std::declval<T>().required_components_archetype());
template<typename T>
using has_register_component_type_t = decltype(std::declval<T>().register_component_type(std::declval<System*>()));
}



class ComponentBoxBase : NonMovable {
    public:
        virtual ~ComponentBoxBase();

        virtual ComponentRuntimeInfo runtime_info() const = 0;
        virtual void add_to(EntityWorld& world, EntityId id) const = 0;
        // virtual void add_or_replace_to(EntityWorld& world, EntityId id) const = 0;

        y_serde3_poly_abstract_base(ComponentBoxBase)
};

template<typename T>
class ComponentBox final : public ComponentBoxBase {
    public:
        ComponentBox() = default;
        ComponentBox(T t);

        ComponentRuntimeInfo runtime_info() const override;
        void add_to(EntityWorld& world, EntityId id) const override;
        // void add_or_replace_to(EntityWorld& world, EntityId id) const override;

        const T& component() const {
            return _component;
        }

        y_reflect(ComponentBox, _component)
        y_serde3_poly(ComponentBox)

    private:
        T _component;
};



class ComponentContainerBase : NonMovable {
    public:
        virtual ~ComponentContainerBase();

        virtual void register_component_type(System* system) const  = 0;

        virtual ComponentRuntimeInfo runtime_info() const = 0;

        virtual void add(EntityWorld& world, EntityId id) = 0;

        virtual std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const = 0;


        inline bool contains(EntityId id) const {
            return id_set().contains(id);
        }

        inline core::Span<EntityId> ids() const {
            return id_set().ids();
        }

        inline ComponentTypeIndex type_id() const {
            return _type_id;
        }

        inline const SparseIdSetBase& id_set() const {
            return *reinterpret_cast<const SparseIdSetBase*>(this + 1);
        }

        inline const SparseIdSet& recently_mutated() const {
            return _mutated;
        }

        inline const SparseIdSet& to_be_removed() const {
            return _to_remove;
        }


        void remove(EntityId id) {
            if(contains(id)) {
                _to_remove.insert(id);
            }
        }

        template<typename T, typename... Args>
        inline T& add(EntityWorld& world, EntityId id, Args&&... args) {
            _mutated.insert(id);

            auto& set = component_set<T>();
            if(!set.contains_index(id.index())) {
                add_required_components<T>(world, id);
                return set.insert(id, y_fwd(args)...);
            } else {
                if constexpr(sizeof...(Args)) {
                    return set[id] = T{y_fwd(args)...};
                } else {
                    return set[id] = T();
                }
            }
        }


        template<typename T>
        inline auto* component_ptr(EntityId id) {
            if constexpr(traits::is_component_mutable_v<T>) {
                _mutated.insert(id);
                return component_set<T>().try_get(id);
            } else {
                return const_component_set<T>().try_get(id);
            }
        }

        template<typename T>
        inline const auto* component_ptr(EntityId id) const {
            return component_set<T>().try_get(id);
        }

        template<typename T>
        inline core::Span<T> components() const {
            return component_set<T>().values();
        }



        y_serde3_poly_abstract_base(ComponentContainerBase)

    private:
        friend class EntityWorld;

        const ComponentTypeIndex _type_id;
        SparseIdSet _mutated;
        SparseIdSet _to_remove;

    protected:
        ComponentContainerBase(ComponentTypeIndex type_id) : _type_id(type_id) {
        }

        virtual void clean_after_tick();
        virtual void prepare_for_tick();

        template<typename T>
        void add_required_components(EntityWorld& world, EntityId id);


    private:
        template<typename T>
        inline auto& component_set() {
            return component_set_fast<traits::component_raw_type_t<T>>();
        }

        template<typename T>
        inline const auto& component_set() const {
            return component_set_fast<traits::component_raw_type_t<T>>();
        }

        template<typename T>
        inline const auto& const_component_set() const {
            return component_set_fast<traits::component_raw_type_t<T>>();
        }

        // Filthy hack to avoid having to cast to ComponentContainer<T> when we already know T
        template<typename T>
        inline auto& component_set_fast() {
            y_debug_assert(type_index<T>() == _type_id);
            return *reinterpret_cast<SparseComponentSet<T>*>(this + 1);
        }

        template<typename T>
        inline const auto& component_set_fast() const {
            y_debug_assert(type_index<T>() == _type_id);
            return *reinterpret_cast<const SparseComponentSet<T>*>(this + 1);
        }
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
    public:
        using component_type = T;

        ComponentContainer() : ComponentContainerBase(type_index<T>()) {
            static_assert(std::is_same_v<traits::component_raw_type_t<T>, T>);
            static_assert(sizeof(*this) == sizeof(ComponentContainerBase) + sizeof(_components));
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

        void add(EntityWorld& world, EntityId id) override {
            ComponentContainerBase::add<T>(world, id);
        }

        std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const override {
            unused(id);
            if constexpr(std::is_copy_constructible_v<T>) {
                return std::make_unique<ComponentBox<T>>(_components[id]);
            }
            return nullptr;
        }


        y_no_serde3_expr(serde3::has_no_serde3_v<T>)

        y_reflect(ComponentContainer, _components)
        y_serde3_poly(ComponentContainer)

    protected:
        void clean_after_tick() override {
            y_profile();
            for(const EntityId id : to_be_removed()) {
                _components.erase(id);
            }
            ComponentContainerBase::clean_after_tick();
        }

    private:
        SparseComponentSet<T> _components;
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H


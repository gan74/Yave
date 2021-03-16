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
#ifndef YAVE_ECS_COMPONENTCONTAINER_H
#define YAVE_ECS_COMPONENTCONTAINER_H

#include "ecs.h"
#include "SparseComponentSet.h"
#include "ComponentRuntimeInfo.h"

Y_TODO(try replacing this?)
#include <y/serde3/archives.h>

namespace yave {
namespace ecs {

namespace detail {
template<typename T>
using has_required_components_t = decltype(std::declval<T>().required_components_archetype());
}



class ComponentBoxBase : NonMovable {
    public:
        virtual ~ComponentBoxBase();

        virtual ComponentRuntimeInfo runtime_info() const = 0;
        virtual void add_to(EntityWorld& world, EntityId id) const = 0;

        y_serde3_poly_abstract_base(ComponentBoxBase)
};

template<typename T>
class ComponentBox final : public ComponentBoxBase {
    public:
        ComponentBox() = default;
        ComponentBox(T t);

        ComponentRuntimeInfo runtime_info() const override;
        void add_to(EntityWorld& world, EntityId id) const override;

        y_reflect(_component)
        y_serde3_poly(ComponentBox)

    private:
        T _component;
};



class ComponentContainerBase : NonMovable {
    public:
        virtual ~ComponentContainerBase();

        bool contains(EntityId id) const;
        core::Span<EntityId> ids() const;

        ComponentTypeIndex type_id() const;

        virtual ComponentRuntimeInfo runtime_info() const = 0;

        virtual void add(EntityWorld& world, EntityId id) = 0;
        virtual void remove(EntityId id) = 0;

        virtual std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const = 0;



        template<typename T, typename... Args>
        T& add(EntityWorld& world, EntityId id, Args&&... args) {
            auto& set = component_set_fast<T>();
            if(!set.contains_index(id.index())) {
                add_required_components<T>(world, id);
                _recently_added.emplace_back(id);
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
        T& component(EntityId id) {
            return component_set_fast<T>()[id];
        }

        template<typename T>
        const T& component(EntityId id) const {
            return component_set_fast<T>()[id];
        }

        template<typename T>
        T* component_ptr(EntityId id) {
            return component_set_fast<T>().try_get(id);
        }

        template<typename T>
        const T* component_ptr(EntityId id) const {
            return component_set_fast<T>().try_get(id);
        }

        template<typename T>
        core::MutableSpan<T> components() {
            return component_set_fast<T>().values();
        }

        template<typename T>
        core::Span<T> components() const {
            return component_set_fast<T>().values();
        }

        template<typename T>
        SparseComponentSet<T>& component_set() {
            return component_set_fast<T>();
        }

        template<typename T>
        const SparseComponentSet<T>& component_set() const {
            return component_set_fast<T>();
        }

        core::Span<EntityId> recently_added() const {
            return _recently_added;
        }


        y_serde3_poly_abstract_base(ComponentContainerBase)

    protected:
        template<typename T>
        ComponentContainerBase(SparseComponentSet<T>& sparse) :
                _sparse(&sparse),
                _ids(&sparse),
                _type_id(type_index<T>()) {
        }


        template<typename T>
        void add_required_components(EntityWorld& world, EntityId id);

    private:
        friend class EntityWorld;

        void clear_recent() {
            return _recently_added.make_empty();
        }

    private:
        // hacky but avoids a bunch of dynamic casts and virtual calls
        void* _sparse = nullptr;
        SparseIdSet* _ids = nullptr;

        const ComponentTypeIndex _type_id;

        Y_TODO(make better version of this)
        core::Vector<EntityId> _recently_added;


        template<typename T>
        auto& component_set_fast() {
            y_debug_assert(_sparse);
            y_debug_assert(type_index<T>() == _type_id);
            return *static_cast<SparseComponentSet<T>*>(_sparse);
        }

        template<typename T>
        const auto& component_set_fast() const {
            y_debug_assert(_sparse);
            y_debug_assert(type_index<T>() == _type_id);
            return *static_cast<const SparseComponentSet<T>*>(_sparse);
        }
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
    public:
        ComponentContainer() : ComponentContainerBase(_components) {
        }

        ComponentRuntimeInfo runtime_info() const override {
            return ComponentRuntimeInfo::create<T>();
        }

        void add(EntityWorld& world, EntityId id) override {
            ComponentContainerBase::add<T>(world, id);
        }

        void remove(EntityId id) override {
            if(_components.contains(id)) {
                _components.erase(id);
            }
        }

        std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const override {
            unused(id);
            if constexpr(std::is_copy_constructible_v<T>) {
                return std::make_unique<ComponentBox<T>>(_components[id]);
            }
            return nullptr;
        }

        y_no_serde3_expr(serde3::has_no_serde3_v<T> || (!serde3::has_serde3_v<T> && !serde3::is_pod_v<T>))

        y_reflect(_components)
        y_serde3_poly(ComponentContainer)


    private:
        SparseComponentSet<T> _components;
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H


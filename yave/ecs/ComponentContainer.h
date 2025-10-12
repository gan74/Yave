/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#include "ComponentBox.h"

#include <y/serde3/poly.h>
#include <y/serde3/traits.h>

// #include <y/utils/log.h>
// #include <y/utils/format.h>

namespace yave {
namespace ecs {

class ComponentContainerBase : NonMovable {

    public:
        virtual ~ComponentContainerBase();

        ComponentTypeIndex type_id() const;

        const SparseIdSet& mutated_ids() const;
        const SparseIdSet& pending_deletions() const;

        usize requirements_chain_depth() const;

        virtual void remove_later(EntityId id) = 0;
        virtual void add_if_not_exist(EntityId id) = 0;

        virtual ComponentRuntimeInfo runtime_info() const = 0;
        virtual void inspect_component(EntityId id, ComponentInspector* inspector) = 0;

        virtual std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const = 0;


        y_serde3_poly_abstract_base(ComponentContainerBase)

    protected:
        friend class EntityWorld;

        template<typename... Ts>
        friend class EntityGroup;

        ComponentContainerBase(ComponentTypeIndex type_id);

        void add_required_components(EntityId id) const;
        bool is_component_required(EntityId id) const;

        virtual void register_component_type(System* system) const = 0;
        virtual void post_load() = 0;
        virtual void process_deferred_changes() = 0;


        const ComponentTypeIndex _type_id;
        ComponentMatrix* _matrix = nullptr;
        SparseIdSet _mutated;
        SparseIdSet _to_delete;
        core::FixedArray<ComponentContainerBase*> _required;
        core::Vector<ComponentTypeIndex> _required_by;

        ProfiledSharedLock<> _lock;

    public:
        using lock_type = decltype(_lock);
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {

    public:
        using component_type = T;

        ComponentContainer() : ComponentContainerBase(type_index<T>()) {
            y_profile_set_lock_name(_lock, fmt_c_str("Lock<{}>", ComponentRuntimeInfo::create<T>().clean_component_name()));
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
                _mutated.insert(id);
                _to_delete.erase(id);
                return &(_components[id] = T(y_fwd(args)...));
            }
            return add(id, y_fwd(args)...);
        }

        void remove_later(EntityId id) override {
            if(_components.contains(id)) {
                _to_delete.insert(id);
            }
        }

        void add_if_not_exist(EntityId id) override {
            get_or_add(id);
        }

        ComponentRuntimeInfo runtime_info() const override {
            return ComponentRuntimeInfo::create<T>();
        }

        void inspect_component(EntityId id, ComponentInspector* inspector) override {
            if(!_components.contains(id)) {
                return;
            }

            Y_TODO(lock??)

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

        std::unique_ptr<ComponentBoxBase> create_box(EntityId id) const override {
            const T* comp = try_get(id);
            if(!comp) {
                return nullptr;
            }

            return std::make_unique<ComponentBox<T>>(*comp);
        }


        y_serde3_poly(ComponentContainer)
        y_reflect(ComponentContainer, _components)

        y_no_serde3_expr(serde3::has_no_serde3<T>);

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

            for(const EntityId id : _components.ids()) {
                y_debug_assert(_matrix->has_all_required_components<T>(id));
            }
        }

        void process_deferred_changes() override {
            for(const EntityId id : _to_delete) {
                if(is_component_required(id)) {
                    // log_msg(fmt("{} #{} can't be removed as it is required by other components", runtime_info().clean_component_name(), id.index()), Log::Warning);
                    continue;
                }

                _matrix->remove_component<T>(id);
                _components.erase(id);
            }
            _to_delete.make_empty();
            _mutated.make_empty();
        }



        template<typename... Args>
        T* add(EntityId id, Args&... args) {
            add_required_components(id);
            T* component =  &_components.insert(id, y_fwd(args)...);
            _matrix->add_component<T>(id);
            _mutated.insert(id);
            return component;
        }


        SparseComponentSet<T> _components;
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H


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
#ifndef YAVE_ECS_ENTITYWORLD_H
#define YAVE_ECS_ENTITYWORLD_H

#include "EntityPool.h"
#include "Query.h"
#include "Archetype.h"
#include "EntityPrefab.h"
#include "System.h"
#include "tags.h"

#include "WorldComponentContainer.h"
#include "ComponentContainer.h"

#include <y/core/ScratchPad.h>

namespace yave {
namespace ecs {

class EntityWorld {
    public:
        EntityWorld();
        ~EntityWorld();

        EntityWorld(EntityWorld&& other);
        EntityWorld& operator=(EntityWorld&& other);

        void swap(EntityWorld& other);

        void tick();
        void update(float dt);

        usize entity_count() const;
        bool exists(EntityId id) const;

        EntityId create_entity();
        EntityId create_entity(const Archetype& archetype);
        EntityId create_entity(const EntityPrefab& prefab);

        void remove_entity(EntityId id);

        EntityId id_from_index(u32 index) const;

        EntityPrefab create_prefab(EntityId id) const;

        core::Span<EntityId> component_ids(ComponentTypeIndex type_id) const;
        core::Span<EntityId> recently_mutated(ComponentTypeIndex type_id) const;
        core::Span<EntityId> to_be_removed(ComponentTypeIndex type_id) const;

        core::Span<EntityId> with_tag(const core::String& tag) const;

        const SparseIdSetBase* tag_set(const core::String& tag) const;

        std::string_view component_type_name(ComponentTypeIndex type_id) const;

        void make_mutated(ComponentTypeIndex type_id, core::Span<EntityId> ids);



        // ---------------------------------------- Parent ----------------------------------------

        EntityId parent(EntityId id) const;
        void set_parent(EntityId id, EntityId parent_id);


        // ---------------------------------------- Systems ----------------------------------------

        template<typename S, typename... Args>
        S* add_system(Args&&... args) {
            auto s = std::make_unique<S>(y_fwd(args)...);
            S* system = s.get();
            _systems.emplace_back(std::move(s));
            register_component_types(system);
            system->_world = this;
            system->setup();
            return system;
        }

        template<typename S>
        const S* find_system() const {
            for(auto& system : _systems) {
                if(const S* s = dynamic_cast<const S*>(system.get())) {
                    return s;
                }
            }
            return nullptr;
        }

        template<typename S>
        S* find_system() {
            for(auto& system : _systems) {
                if(S* s = dynamic_cast<S*>(system.get())) {
                    return s;
                }
            }
            return nullptr;
        }



        // ---------------------------------------- Static Archetypes ----------------------------------------

        template<typename... Args>
        EntityId create_entity() {
            const EntityId id = create_entity();
            add_or_replace_components<Args...>(id);
            return id;
        }



        // ---------------------------------------- Components ----------------------------------------

        template<typename T>
        T* get_or_add_component(EntityId id) {
            check_exists(id);
            return &find_container<T>()->template get_or_add<T>(id);
        }

        template<typename T, typename... Args>
        T* add_or_replace_component(EntityId id, Args&&... args) {
            check_exists(id);
            return &find_container<T>()->template add_or_replace<T>(id, y_fwd(args)...);
        }

        template<typename First, typename... Args>
        void add_or_replace_components(EntityId id) {
            y_debug_assert(exists(id));
            if(!has<First>(id)) {
                add_or_replace_component<First>(id);
            }
            if constexpr(sizeof...(Args) != 0) {
                add_or_replace_components<Args...>(id);
            }
        }



        // ---------------------------------------- Tags ----------------------------------------

        void add_tag(EntityId id, const core::String& tag);

        void remove_tag(EntityId id, const core::String& tag);

        void clear_tag(const core::String& tag);

        bool has_tag(EntityId id, const core::String& tag) const;

        static bool is_tag_implicit(std::string_view tag);




        // ---------------------------------------- Enumerations ----------------------------------------

        auto ids() const {
            return _entities.ids();
        }

        auto component_types() const {
            auto tr = [](const std::unique_ptr<ComponentContainerBase>& cont) { return cont->type_id(); };
            return core::Range(TransformIterator(_containers.begin(), tr), _containers.end());
        }

        auto tags() const {
            return _tags.keys();
        }

        core::Span<std::unique_ptr<System>> systems() const {
           return _systems;
        }



        // ---------------------------------------- Component getters ----------------------------------------

        template<typename T>
        bool has(EntityId id) const {
            return find_container<T>()->contains(id);
        }

        bool has(EntityId id, ComponentTypeIndex type_id) const {
            return find_container(type_id)->contains(id);
        }

        template<typename T>
        auto* component(EntityId id) {
            return find_container<T>()->template component_ptr<T>(id);
        }

        template<typename T>
        auto* component_mut(EntityId id) {
            return component<Mutate<T>>(id);
        }

        template<typename T>
        const auto* component(EntityId id) const {
            return find_container<T>()->template component_ptr<T>(id);
        }



        // ---------------------------------------- World Components ----------------------------------------

        template<typename T, typename... Args>
        T* get_or_add_world_component(Args&&... args) {
            for(auto& container : _world_components) {
                if(auto* t = container->try_get<T>()) {
                    return t;
                }
            }
            auto& ptr = _world_components.emplace_back(std::make_unique<WorldComponentContainer<T>>(y_fwd(args)...));
            return ptr->template try_get<T>();
        }

        template<typename T>
        T* world_component() {
            for(auto& container : _world_components) {
                if(auto* t = container->try_get<T>()) {
                    return t;
                }
            }
            return nullptr;
        }

        template<typename T>
        const T* world_component() const {
            for(auto& container : _world_components) {
                if(auto* t = container->try_get<T>()) {
                    return t;
                }
            }
            return nullptr;
        }



        // ---------------------------------------- Component sets ----------------------------------------

        template<typename T>
        const SparseComponentSet<T>& component_set() const {
            return find_container<T>()->template component_set<T>();
        }

        template<typename T>
        core::Span<T> components() const {
            return find_container<T>()->template components<T>();
        }

        template<typename T>
        core::Span<EntityId> component_ids() const {
            return component_ids(type_index<T>());
        }

        template<typename T>
        core::Span<EntityId> recently_mutated() const {
            return recently_mutated(type_index<T>());
        }

        template<typename T>
        core::Span<EntityId> to_be_removed() const {
            return to_be_removed(type_index<T>());
        }



        // ---------------------------------------- Queries ----------------------------------------

        template<typename... Args>
        auto query(core::Span<core::String> tags = {}) {
            auto q = Query<Args...>(typed_component_sets_or_none<Args...>(), build_matches_for_query<Args...>(tags));
            dirty_mutated_containers<Args...>(q.ids());
            return q;
        }

        template<typename... Args>
        auto query(core::Span<core::String> tags = {}) const {
            static_assert((traits::is_component_const_v<Args> && ...));
            auto q = Query<Args...>(typed_component_sets_or_none<Args...>(), build_matches_for_query<Args...>(tags));

            return q;
        }

        template<typename... Args>
        auto query(core::Span<EntityId> ids, core::Span<core::String> tags = {}) {
            auto q = Query<Args...>(typed_component_sets_or_none<Args...>(), build_matches_for_query<Args...>(tags), ids);
            dirty_mutated_containers<Args...>(q.ids());
            return q;
        }

        template<typename... Args>
        auto query(core::Span<EntityId> ids, core::Span<core::String> tags = {}) const {
            static_assert((traits::is_component_const_v<Args> && ...));
            auto q = Query<Args...>(typed_component_sets_or_none<Args...>(), build_matches_for_query<Args...>(tags), ids);

            return q;
        }



        // ---------------------------------------- Misc ----------------------------------------

        template<typename T>
        void make_mutated(core::Span<EntityId> ids) {
            make_mutated(type_index<T>(), ids);
        }

        void inspect_components(EntityId id, ComponentInspector* inspector);

        void post_deserialize();


        y_reflect(EntityWorld, _entities, _containers, _tags, _world_components)

    private:
        template<typename T>
        friend class ComponentContainer;


        template<typename T>
        const ComponentContainerBase* find_container() const {
            using component_type = traits::component_raw_type_t<T>;

            static const auto static_info = ComponentRuntimeInfo::create<component_type>();
            unused(static_info);
            return find_container(type_index<component_type>());
        }

        template<typename T>
        ComponentContainerBase* find_container() {
            using component_type = traits::component_raw_type_t<T>;

            static const auto static_info = ComponentRuntimeInfo::create<component_type>();
            unused(static_info);
            return find_container(type_index<component_type>());
        }

        template<typename T, typename... Args>
        auto typed_component_sets() const {
            if constexpr(sizeof...(Args) != 0) {
                return std::tuple_cat(typed_component_sets<T>(),
                                      typed_component_sets<Args...>());
            } else {
                // We need non consts here and we want to avoir returning non const everywhere else
                // This shouldn't be UB as component containers are never const
                using component_type = traits::component_raw_type_t<T>;
                ComponentContainerBase* container = const_cast<ComponentContainerBase*>(find_container<component_type>());
                auto* set = container ? &container->component_set<component_type>() : nullptr;
                return std::tuple{set};
            }
        }

        template<typename... Args>
        auto typed_component_sets_or_none() const {
            if constexpr(sizeof...(Args) != 0) {
                return typed_component_sets<Args...>();
            } else {
                return std::tuple<>{};
            }
        }

        template<usize I = 0, typename... Args>
        auto fill_container_array(std::array<const ComponentContainerBase*, sizeof...(Args)>& containers) const {
            if constexpr(I < sizeof...(Args)) {
                using component_type = typename traits::component_raw_type_t<std::tuple_element_t<I, std::tuple<Args...>>>;
                containers[I] = find_container<component_type>();
                fill_container_array<I + 1, Args...>(containers);
            }
        }

        template<typename... Args>
        auto build_matches_for_query(core::Span<core::String> tags) const {
            constexpr usize container_count = sizeof...(Args);

            std::array<const ComponentContainerBase*, container_count> containers;
            fill_container_array<0, Args...>(containers);

            core::ScratchPad<QueryUtils::SetMatch> matches(container_count + tags.size());
            QueryUtils::fill_match_array<0, Args...>(matches, containers);

            for(usize i = 0; i != tags.size(); ++i) {
                const bool is_neg = tags[i].starts_with("!");
                matches[container_count + i] = {
                    tag_set(is_neg ? core::String(tags[i].sub_str(1)) : tags[i]),
                    !is_neg
                };
            }

            return matches;
        }


        template<typename T, typename... Args>
        void dirty_mutated_containers(core::Span<EntityId> mutated) {
            if(mutated.is_empty()) {
                return;
            }
            if constexpr(sizeof...(Args) != 0) {
                dirty_mutated_containers<T>(mutated);
                dirty_mutated_containers<Args...>(mutated);
            } else {
                if constexpr(traits::is_component_mutable_v<T>) {
                    using component_type = traits::component_raw_type_t<T>;
                    make_mutated<component_type>(mutated);
                }
            }
        }



        const SparseIdSet* raw_tag_set(const core::String& tag) const;

        const ComponentContainerBase* find_container(ComponentTypeIndex type_id) const;
        ComponentContainerBase* find_container(ComponentTypeIndex type_id);

        void register_component_types(System* system) const;

        void check_exists(EntityId id) const;


        core::Vector<std::unique_ptr<ComponentContainerBase>> _containers;
        core::FlatHashMap<core::String, SparseIdSet> _tags;
        EntityPool _entities;

        core::Vector<std::unique_ptr<System>> _systems;
        core::Vector<std::unique_ptr<WorldComponentContainerBase>> _world_components;
};

}
}

#include "EntityWorld.inl"

#endif // YAVE_ECS_ENTITYWORLD_H


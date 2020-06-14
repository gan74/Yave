/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include "EntityIdPool.h"
#include "EntityView.h"
#include "Archetype.h"
#include "EntityPrefab.h"

#include "ComponentContainer.h"

namespace yave {
namespace ecs {

class EntityWorld {
	public:
		EntityWorld();
		~EntityWorld();

		EntityWorld(EntityWorld&& other);
		EntityWorld& operator=(EntityWorld&& other);

		void swap(EntityWorld& other);

		usize entity_count() const;
		bool exists(EntityId id) const;

		EntityId create_entity();
		EntityId create_entity(const Archetype& archetype);
		EntityId create_entity(const EntityPrefab& prefab);

		void remove_entity(EntityId id);

		EntityId id_from_index(u32 index) const;

		EntityPrefab create_prefab(EntityId id) const;

		core::Span<ComponentTypeIndex> required_components() const;

		std::string_view component_type_name(ComponentTypeIndex type_id) const;



		template<typename... Args>
		EntityId create_entity(StaticArchetype<Args...>) {
			const EntityId id = create_entity();
			add_components<Args...>(id);
			return id;
		}


		template<typename T, typename... Args>
		void add_component(EntityId id, Args&&... args) {
			check_exists(id);
			find_or_create_container<T>()->template add<T>(*this, id, y_fwd(args)...);
		}

		template<typename First, typename... Args>
		void add_components(EntityId id) {
			y_debug_assert(exists(id));
			add_component<First>(id);
			if constexpr(sizeof...(Args)) {
				add_components<Args...>(id);
			}
		}


		auto ids() const {
			return _entities.ids();
		}

		auto component_types() const {
			auto tr = [](const std::unique_ptr<ComponentContainerBase>& cont) { return cont->type_id(); };
			return core::Range(TransformIterator(_containers.values().begin(), tr),
							   _containers.values().end());
		}


		template<typename T>
		bool has(EntityId id) const {
			const ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->contains(id) : false;
		}

		template<typename T>
		T* component(EntityId id) {
			ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->template component_ptr<T>(id) : nullptr;
		}

		template<typename T>
		const T* component(EntityId id) const {
			const ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->template component_ptr<T>(id) : nullptr;
		}


		template<typename T>
		core::MutableSpan<T> components() {
			ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->components<T>() : decltype(cont->components<T>())();
		}

		template<typename T>
		core::Span<T> components() const {
			const ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->components<T>() : decltype(cont->components<T>())();
		}

		template<typename T>
		core::Span<EntityId> component_ids() const {
			const ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->ids() : core::Span<EntityId>();
		}


		template<typename... Args>
		EntityView<false, Args...> view() {
			static_assert(sizeof...(Args));
			return EntityView<false, Args...>(typed_component_sets<Args...>());
		}

		template<typename... Args>
		EntityView<true, Args...> view() const {
			static_assert(sizeof...(Args));
			return EntityView<true, Args...>(typed_component_sets<Args...>());
		}

		template<typename... Args>
		auto view(StaticArchetype<Args...>) {
			return view<Args...>();
		}

		template<typename... Args>
		auto view(StaticArchetype<Args...>) const {
			return view<Args...>();
		}

		template<typename T>
		void add_required_component() {
			static_assert(std::is_default_constructible_v<T>);
			Y_TODO(check for duplicates)
			_required_components << find_or_create_container<T>()->type_id();
			for(const ComponentTypeIndex c : _required_components) {
				y_debug_assert(find_container(c)->type_id() == c);
			}
			for(EntityId id : ids()) {
				add_component<T>(id);
			}
		}


		void flush_reload(AssetLoader& loader);

		void post_deserialize();

		y_serde3(_entities, _containers)

	private:
		template<typename T>
		friend class ComponentContainer;


		template<typename T>
		const ComponentContainerBase* find_container() const {
			return find_container(type_index<T>());
		}

		template<typename T>
		ComponentContainerBase* find_container() {
			return find_container(type_index<T>());
		}

		template<typename T>
		ComponentContainerBase* find_or_create_container() {
			auto& cont = _containers[type_index<T>()];
			if(!cont) {
				cont = std::make_unique<ComponentContainer<T>>();
			}
			return cont.get();
		}


		// This is not const correct, but we don't expose it so it's fine
		template<typename T, typename... Args>
		std::tuple<SparseComponentSet<T>*, SparseComponentSet<Args>*...> typed_component_sets() const {
			if constexpr(sizeof...(Args)) {
				return std::tuple_cat(typed_component_sets<T>(),
									  typed_component_sets<Args...>());
			} else {
				// We need non consts here and we want to avoir returning non const everywhere else
				// This shouldn't be UB as component containers are never const
				ComponentContainerBase* cont = const_cast<ComponentContainerBase*>(find_container<T>());
				return cont ? &cont->component_set<T>() : nullptr;
			}
		}


		const ComponentContainerBase* find_container(ComponentTypeIndex type_id) const;
		ComponentContainerBase* find_container(ComponentTypeIndex type_id);
		ComponentContainerBase* find_or_create_container(const ComponentRuntimeInfo& info);

		void check_exists(EntityId id) const;


		core::ExternalHashMap<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>> _containers;
		EntityIdPool _entities;

		core::Vector<ComponentTypeIndex> _required_components;
};

}
}

#include "EntityWorld.inl"

#endif // YAVE_ECS_ENTITYWORLD_H

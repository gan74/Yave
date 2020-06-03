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
#ifndef Y_ECS_ENTITYWORLD_H
#define Y_ECS_ENTITYWORLD_H

#include "EntityIDPool.h"
#include "EntityView.h"
#include "Archetype.h"
#include "EntityPrefab.h"

#include "ComponentContainer.h"

namespace y {
namespace ecs {

class EntityWorld : NonMovable {
	public:
		EntityWorld();

		usize entity_count() const;
		bool exists(EntityID id) const;

		EntityID create_entity();
		EntityID create_entity(const Archetype& archetype);
		EntityID create_entity(const EntityPrefab& prefab);

		void remove_entity(EntityID id);


		EntityPrefab create_prefab(EntityID id) const;


		std::string_view component_type_name(ComponentTypeIndex type_id) const;


		template<typename T, typename... Args>
		void add_component(EntityID id, Args&&... args) {
			check_exists(id);
			find_or_create_container<T>()->template add<T>(id, y_fwd(args)...);
		}

		template<typename First, typename... Args>
		void add_components(EntityID id) {
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
		T* component(EntityID id) {
			ComponentContainerBase* cont = find_container<T>();
			return cont ? cont->template component_ptr<T>(id) : nullptr;
		}

		template<typename T>
		const T* component(EntityID id) const {
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



		template<typename... Args>
		EntityView<false, Args...> view() {
			static_assert(sizeof...(Args));
			return typed_component_sets<Args...>();
		}

		template<typename... Args>
		EntityView<true, Args...> view() const {
			static_assert(sizeof...(Args));
			return typed_component_sets<Args...>();
		}

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
				cont = std::make_unique<ComponentContainer<T>>(this);
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

		void check_exists(EntityID id) const;


		core::ExternalHashMap<u32, std::unique_ptr<ComponentContainerBase>> _containers;
		EntityIDPool _entities;
};


template<typename... Args>
void RequiredComponents<Args...>::add_required_components(EntityWorld& world, EntityID id) {
	world.add_components<Args...>(id);
}

}
}

#endif // Y_ECS_ENTITYWORLD_H

/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "Entity.h"
#include "ComponentContainer.h"
#include "MultiComponentIterator.h"

#include <y/core/Result.h>

#include <unordered_map>

namespace yave {
namespace ecs {

class EntityWorld : NonCopyable {

	public:
		EntityWorld();

		const Entity* entity(EntityId id) const;
		Entity* entity(EntityId id);

		EntityId create_entity();
		void remove_entity(EntityId id);

		void flush();



		const auto& entities() const {
			return _entities;
		}




		template<typename T, typename... Args>
		TypedComponentId<T> create_component(EntityId id, Args&&... args) {
			return create_component(typed_component_container<T>(), id, y_fwd(args)...);
		}

		template<typename T>
		void remove_component(EntityId id) {
			return remove_component(component_container<T>(), id);
		}




		template<typename T>
		T* component(TypedComponentId<T> id) {
			return component<T>(id.to_generic_id());
		}

		template<typename T>
		T* component(ComponentId id) {
			return typed_component_container<T>()->component(id);
		}

		template<typename T>
		T* component(EntityId id) {
			return component<T>(entity(id)->component_id(add_index_for_type<T>()));
		}

		template<typename T>
		const T* component(TypedComponentId<T> id) const {
			return component<T>(id.to_generic_id());
		}

		template<typename T>
		const T* component(ComponentId id) const {
			ComponentContainer<T>* container = typed_component_container<T>();
			return container ? container->component(id) : nullptr;
		}

		template<typename T>
		const T* component(EntityId id) const {
			if(auto index = index_for_type<T>()) {
				return component<T>(entity(id)->component_id(index.unwrap()));
			}
			return nullptr;
		}




		template<typename... Args>
		auto entities_with() {
			return with_components<Args...>(ReturnEntityPolicy());
		}

		template<typename... Args>
		auto entities_with() const {
			return with_components<Args...>(ReturnEntityPolicy());
		}


		template<typename... Args>
		auto ids_with() {
			return with_components<Args...>(ReturnIdPolicy());
		}

		template<typename... Args>
		auto ids_with() const {
			return with_components<Args...>(ReturnIdPolicy());
		}


		template<typename T, typename... Args>
		auto components() {
			if constexpr(sizeof...(Args)) {
				return with_components<T, Args...>(ReturnComponentsPolicy<false, T, Args...>(typed_component_containers<T, Args...>()));
			} else  {
				return typed_component_container<T>()->components();
			}
		}

		template<typename T, typename... Args>
		auto components() const {
			if constexpr(sizeof...(Args)) {
				return with_components<T, Args...>(ReturnComponentsPolicy<true, T, Args...>(typed_component_containers<T, Args...>()));
			} else  {
				const ComponentContainer<T>* container = typed_component_container<T>();
				return container ? container->components() : ComponentContainer<T>::empty_components();
			}
		}




		core::String type_name(TypeIndex index) const;

	private:
		template<typename... Args, typename ReturnPolicy>
		auto with_components(const ReturnPolicy& policy) {
			y_profile();
			static_assert(sizeof...(Args));
			core::ArrayView<EntityId> entities = smallest_container<Args...>()->parents();
			MultiComponentIterator<decltype(entities.begin()), ReturnPolicy, false> begin(
						entities.begin(), entities.end(), _entities, create_bitmask<Args...>(), policy);
			return core::Range(begin, MultiComponentIteratorEndSentry());
		}

		template<typename... Args, typename ReturnPolicy>
		auto with_components(const ReturnPolicy& policy) const {
			y_profile();
			static_assert(sizeof...(Args));
			core::ArrayView<EntityId> entities = smallest_container<Args...>()->parents();
			MultiComponentIterator<decltype(entities.begin()), ReturnPolicy, true> begin(
						entities.begin(), entities.end(), _entities, create_bitmask<Args...>(), policy);
			return core::Range(begin, MultiComponentIteratorEndSentry());
		}


		template<typename T>
		ComponentContainerBase* component_container() {
			TypeIndex type = add_index_for_type<T>();
			auto& container = _component_containers[type.index];
			if(!container) {
				container = std::make_unique<ComponentContainer<T>>(*this, type);
			}
			y_debug_assert(container->type() == type);
			return container.get();
		}

		template<typename T>
		const ComponentContainerBase* component_container() const {
			if(auto index = index_for_type<T>()) {
				if(index.unwrap().index < _component_containers.size()) {
					return _component_containers[index.unwrap().index].get();
				}
			}
			return nullptr;
		}

		template<typename T>
		ComponentContainer<T>* typed_component_container() {
			return dynamic_cast<ComponentContainer<T>*>(component_container<T>());
		}

		template<typename T>
		const ComponentContainer<T>* typed_component_container() const {
			return dynamic_cast<const ComponentContainer<T>*>(component_container<T>());
		}

		template<typename T, typename... Args>
		std::tuple<ComponentContainer<T>*, ComponentContainer<Args>*...> typed_component_containers() const {
			if constexpr(sizeof...(Args)) {
				return std::tuple_cat(typed_component_containers<T>(),
									  typed_component_containers<Args...>());
			} else {
				// We need non consts here and we want to avoir returning non const everywhere else
				// Const safety for typed_component_containers is done in ReturnComponentsPolicy
				// This shouldn't be UB as we component containers are never const
				return std::make_tuple(const_cast<ComponentContainer<T>*>(typed_component_container<T>()));
			}
		}

		template<typename T, typename... Args>
		const ComponentContainerBase* smallest_container() const {
			const ComponentContainerBase* cont = component_container<T>();
			if constexpr(sizeof...(Args)) {
				const ComponentContainerBase* other = smallest_container<Args...>();
				return cont->parents().size() < other->parents().size() ? cont : other;
			}
			return cont;
		}

		template<typename... Args>
		ComponentBitmask create_bitmask() const {
			ComponentBitmask mask;
			add_to_bitmask<Args...>(mask);
			return mask;
		}

		template<typename T, typename... Args>
		void add_to_bitmask(ComponentBitmask& mask) const {
			if(auto index = index_for_type<T>()) {
				mask[index.unwrap().index] = true;
			}
			if constexpr(sizeof...(Args)) {
				add_to_bitmask<Args...>(mask);
			}
		}

		template<typename T>
		TypeIndex add_index_for_type() {
			return add_index_for_type(typeid(T));
		}

		template<typename T>
		core::Result<TypeIndex> index_for_type() const {
			return index_for_type(typeid(T));
		}



		template<typename T, typename... Args>
		ComponentId create_component(ComponentContainer<T>* container, EntityId id, Args&&... args) {
			Entity* ent = entity(id);
			y_debug_assert(ent);

			TypeIndex type = container->type();
			if(ent->has_component(type)) {
				y_fatal("Component already exists.");
			}
			ComponentId comp = container->create_component(id, y_fwd(args)...);
			ent->add_component(type, comp);
			return comp;
		}

		void remove_component(ComponentContainerBase* container, EntityId id);



		TypeIndex add_index_for_type(std::type_index type);
		core::Result<TypeIndex> index_for_type(std::type_index type) const;



		SlotMap<Entity, EntityTag> _entities;
		core::Vector<EntityId> _deletions;

		core::Vector<std::unique_ptr<ComponentContainerBase>> _component_containers;
		std::unordered_map<std::type_index, TypeIndex> _component_type_indexes;
};

}
}


#endif // YAVE_ECS_ENTITYWORLD_H

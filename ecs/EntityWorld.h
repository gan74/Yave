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

#include <unordered_map>

namespace yave {
namespace ecs {

class EntityWorld {

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


		template<typename T>
		ComponentId add_component(EntityId id) {
			return add_component(component_container<T>(), id);
		}

		template<typename T>
		void remove_component(ComponentId id) {
			return add_component(component_container<T>(), id);
		}



		template<typename T>
		const T* component(ComponentId id) const {
			return typed_component_container<T>()->component(id);
		}

		template<typename T>
		T* component(ComponentId id) {
			return typed_component_container<T>()->component(id);
		}

		template<typename T>
		T* component(EntityId id) {
			return component<T>(entity(id)->component_id(index_for_type<T>()));
		}



		template<typename T>
		auto components() {
			return typed_component_container<T>()->components();
		}

		template<typename T>
		auto components() const {
			return typed_component_container<T>()->components();
		}


		template<typename... Args>
		auto entities_with() {
			if constexpr(sizeof...(Args)) {
				core::ArrayView<EntityId> entities = smallest_container<Args...>()->parents();
				ComponentBitmask mask = create_bitmask<Args...>();
				MultiComponentIterator begin(entities.begin(), entities.end(), _entities, mask);
				MultiComponentIteratorEndSentry end;
				return core::Range(begin, end);
			} else {
				return entities();
			}
		}

	private:
		template<typename T>
		ComponentContainerBase* component_container() {
			auto& container = _component_containers[index_for_type<T>().index];
			if(!container) {
				container = std::make_unique<ComponentContainer<T>>();
			}
			return container.get();
		}

		template<typename T>
		ComponentContainer<T>* typed_component_container() {
			return dynamic_cast<ComponentContainer<T>*>(component_container<T>());
		}

		template<typename T, typename... Args>
		ComponentContainerBase* smallest_container() {
			ComponentContainerBase* cont = component_container<T>();
			if constexpr(sizeof...(Args)) {
				ComponentContainerBase* other = smallest_container<Args...>();
				//log_msg(fmt("(%) % (%) %", cont->type().name(), cont->parents().size(), other->type().name(), other->parents().size()));
				return cont->parents().size() < other->parents().size() ? cont : other;
			}
			return cont;
		}

		template<typename... Args>
		ComponentBitmask create_bitmask() {
			ComponentBitmask mask;
			add_to_bitmask<Args...>(mask);
			return mask;
		}

		template<typename T, typename... Args>
		void add_to_bitmask(ComponentBitmask& mask) {
			mask[index_for_type<T>().index] = true;
			if constexpr(sizeof...(Args)) {
				add_to_bitmask<Args...>(mask);
			}
		}

		template<typename T>
		TypeIndex index_for_type() {
			return index_for_type(typeid(T));
		}

		TypeIndex index_for_type(std::type_index type);

		ComponentId add_component(ComponentContainerBase* container, EntityId id);
		void remove_component(ComponentContainerBase* container, ComponentId id);


		SlotMap<Entity, EntityTag> _entities;
		core::Vector<EntityId> _deletions;

		core::Vector<std::unique_ptr<ComponentContainerBase>> _component_containers;
		std::unordered_map<std::type_index, TypeIndex> _component_type_indexes;
};

}
}


#endif // YAVE_ECS_ENTITYWORLD_H

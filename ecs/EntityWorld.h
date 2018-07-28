/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <unordered_map>

namespace yave {
namespace ecs {

class EntityWorld {

	public:
		const Entity* entity(EntityId id) const;
		Entity* entity(EntityId id);

		EntityId create_entity();
		void remove_entity(EntityId id);

		void flush();



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
		auto components() {
			return typed_component_container<T>()->components();
		}

		template<typename T>
		auto components() const {
			return typed_component_container<T>()->components();
		}

	private:
		template<typename T>
		ComponentContainerBase* component_container() {
			auto& container = _component_containers[typeid(T)];
			if(!container) {
				container = std::make_unique<ComponentContainer<T>>();
			}
			return container.get();
		}

		template<typename T>
		ComponentContainer<T>* typed_component_container() {
			return dynamic_cast<ComponentContainer<T>*>(component_container<T>());
		}


		ComponentId add_component(ComponentContainerBase* container, EntityId id);
		void remove_component(ComponentContainerBase* container, ComponentId id);


		FreeList<Entity, EntityTag> _entities;
		core::Vector<EntityId> _deletions;

		std::unordered_map<std::type_index, std::unique_ptr<ComponentContainerBase>> _component_containers;
};

}
}


#endif // YAVE_ECS_ENTITYWORLD_H

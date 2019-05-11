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
#ifndef YAVE_ECS_COMPONENTCONTAINER_H
#define YAVE_ECS_COMPONENTCONTAINER_H

#include "ecs.h"
#include "Entity.h"

#include <typeindex>

#include <y/serde/serde.h>

namespace yave {
namespace ecs {

class ComponentContainerBase : NonMovable {
	public:
		virtual ~ComponentContainerBase();

		virtual void flush() = 0;

		void remove_component(ComponentId id);

		core::ArrayView<EntityId> parents() const;
		EntityId parent(ComponentId id) const;

		TypeIndex type() const;
		const EntityWorld& world() const;

	protected:
		ComponentContainerBase(EntityWorld& world, TypeIndex type);

		void set_parent(ComponentId id, EntityId parent);
		void unset_parent(ComponentId id);

		void finish_flush();

		core::Vector<EntityId> _parents;
		core::Vector<ComponentId> _deletions;

	private:
		EntityWorld& _world;
		TypeIndex _type;
};


namespace detail {
struct RegisteredDeserializer {
	usize hash = 0;
	std::unique_ptr<ComponentContainerBase> (*create)(io::ReaderRef);
	RegisteredDeserializer* next = nullptr;
	static RegisteredDeserializer* head;
};

usize registered_types_count();
}


template<typename T>
class ComponentContainer final : public ComponentContainerBase {

	static struct Registerer {
		Registerer() {
			deser.hash = type_hash<T>();
			deser.create = [](io::ReaderRef) { return std::unique_ptr<ComponentContainerBase>(); };
			deser.next = detail::RegisteredDeserializer::head;
			detail::RegisteredDeserializer::head = &deser;
		}

		detail::RegisteredDeserializer deser;
	} registerer;

	public:

		ComponentContainer(EntityWorld& world, TypeIndex type) :
				ComponentContainerBase(world, type),
				_registerer(&registerer) {
		}

		const T* component(ComponentId id) const {
			return _components.get(id);
		}

		T* component(ComponentId id) {
			return _components.get(id);
		}

		const T* component(const Entity& entity) const {
			return _components.get(entity.component_id(type()));
		}

		T* component(const Entity& entity) {
			return _components.get(entity.component_id(type()));
		}

		template<typename... Args>
		ComponentId create_component(EntityId parent, Args&&... args) {
			ComponentId id = _components.add(y_fwd(args)...);
			set_parent(id, parent);
			return id;
		}

		void flush() override {
			for(ComponentId id : _deletions) {
				_components.remove(id);
			}
			finish_flush();
		}

		auto components() const {
			return core::Range<typename decltype(_components)::const_iterator>(_components);
		}

		auto components() {
			return core::Range<typename decltype(_components)::iterator>(_components);
		}

		static auto empty_components() {
			using const_iterator = typename SlotMap<T, ComponentTag>::const_iterator;
			return core::Range<const_iterator>(const_iterator(), const_iterator());
		}


	private:
		SlotMap<T, ComponentTag> _components;
		Registerer* _registerer = nullptr;

};


template<typename T>
typename ComponentContainer<T>::Registerer ComponentContainer<T>::registerer = ComponentContainer<T>::Registerer();

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

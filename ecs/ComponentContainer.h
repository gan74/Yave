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

#include <typeindex>

namespace yave {
namespace ecs {

class ComponentContainerBase : NonMovable {
	public:
		virtual ~ComponentContainerBase();

		virtual void flush() = 0;

		virtual ComponentId create_component() = 0;
		ComponentId create_component(EntityId parent);
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


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
	public:
		ComponentContainer(EntityWorld& world, TypeIndex type) : ComponentContainerBase(world, type) {
		}

		const T* component(ComponentId id) const {
			return _components.get(id);
		}

		T* component(ComponentId id) {
			return _components.get(id);
		}

		ComponentId create_component() override {
			return _components.add();
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
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

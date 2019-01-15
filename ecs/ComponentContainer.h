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
#ifndef YAVE_ECS_COMPONENTCONTAINER_H
#define YAVE_ECS_COMPONENTCONTAINER_H

#include "ecs.h"

#include <typeindex>

namespace yave {
namespace ecs {

class ComponentContainerBase : NonCopyable {
	public:
		virtual ~ComponentContainerBase();

		virtual void flush() = 0;

		virtual ComponentId create_component() = 0;
		ComponentId create_component(EntityId parent);

		void remove_component(ComponentId id);

		EntityId parent(ComponentId id) const;
		const std::type_index& type() const;

	protected:
		ComponentContainerBase(std::type_index type);


		void set_parent(ComponentId id, EntityId parent);

		core::Vector<EntityId> _parents;
		core::Vector<ComponentId> _deletions;

	private:
		std::type_index _type;
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
	public:
		ComponentContainer() : ComponentContainerBase(typeid(T)) {
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
			_deletions.clear();
		}

		auto components() const {
			return core::Range(_components);
		}

		auto components() {
			return core::Range(_components);
		}

	private:
		SlotMap<T, ComponentTag> _components;
};

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

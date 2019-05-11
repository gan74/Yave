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
using create_container_t = std::unique_ptr<ComponentContainerBase> (*)(io::ReaderRef, EntityWorld&, TypeIndex);
class RegisteredType {
	private:
		friend void register_type(RegisteredType*, usize, create_container_t);
		friend usize registered_types_count();

		usize _hash = 0;
		create_container_t _create_container;
		RegisteredType* _next = nullptr;
};

void register_type(RegisteredType* type, usize hash, create_container_t create_container);
usize registered_types_count();

template<typename T>
void register_type(RegisteredType* type, create_container_t create_container) {
	register_type(type, type_hash<T>(), create_container);
}

}


template<typename T>
class ComponentContainer final : public ComponentContainerBase {

	static struct Registerer {
		Registerer() {
			detail::create_container_t derer_func = nullptr;
			if constexpr(serde::is_deserializable<T>::value) {
				derer_func = [](io::ReaderRef reader, EntityWorld& world, TypeIndex type)
							-> std::unique_ptr<ComponentContainerBase> {
						try {
							auto container = std::make_unique<ComponentContainer<T>>(world, type);
							container->deserialize(reader);
							return container;
						} catch(...) {
						}
						return nullptr;
					};
			}
			detail::register_type<T>(&type, derer_func);
		}

		detail::RegisteredType type;
	} registerer;

	struct MagicNumber {};

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

		y_serde(type_hash<MagicNumber>(), _components)

	private:
		SlotMap<T, ComponentTag> _components;
		// make sure that it is ODR used no matter what
		Registerer* _registerer = nullptr;

};


template<typename T>
typename ComponentContainer<T>::Registerer ComponentContainer<T>::registerer = ComponentContainer<T>::Registerer();

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

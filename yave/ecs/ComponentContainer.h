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

class ComponentContainerBase;

namespace detail {
using create_container_t = std::unique_ptr<ComponentContainerBase> (*)(io::ReaderRef, EntityWorld&);
class RegisteredContainerType {
	public:
		u64 type_id() const {
			return _type_id;
		}
	private:
		friend void register_container_type(RegisteredContainerType*, usize, create_container_t);
		friend usize registered_types_count();
		friend void serialize_container(io::WriterRef, ComponentContainerBase*);
		friend std::unique_ptr<ComponentContainerBase> deserialize_container(io::ReaderRef, EntityWorld&);

		u64 _type_id = 0;
		create_container_t _create_container = nullptr;
		RegisteredContainerType* _next = nullptr;
};

usize registered_types_count();
void register_container_type(RegisteredContainerType* type, u64 type_id, create_container_t create_container);
void serialize_container(io::WriterRef writer, ComponentContainerBase* container);
std::unique_ptr<ComponentContainerBase> deserialize_container(io::ReaderRef reader, EntityWorld& world);

template<typename T>
void register_container_type(RegisteredContainerType* type, create_container_t create_container) {
	register_container_type(type, type_hash<T>(), create_container);
}

template<typename T>
ComponentTypeIndex generate_index_for_type(EntityWorld& world); // EntityWorld.h
}



class ComponentContainerBase : NonMovable {
	public:
		virtual ~ComponentContainerBase();

		virtual void flush() = 0;

		void remove_component(ComponentId id);

		core::ArrayView<EntityId> parents() const;
		EntityId parent(ComponentId id) const;

		ComponentTypeIndex type() const;
		const EntityWorld& world() const;

	protected:
		ComponentContainerBase(EntityWorld& world, ComponentTypeIndex type);

		void set_parent(ComponentId id, EntityId parent);
		void unset_parent(ComponentId id);

		void finish_flush();

		core::Vector<EntityId> _parents;
		core::Vector<ComponentId> _deletions;

	private:
		friend void detail::serialize_container(io::WriterRef, ComponentContainerBase*);

		virtual void serialize(io::WriterRef) const = 0;
		virtual u64 serialization_type_id() const = 0;

	private:
		EntityWorld& _world;
		ComponentTypeIndex _type;
};

template<typename T>
class ComponentContainer final : public ComponentContainerBase {
	public:
		ComponentContainer(EntityWorld& world) :
				ComponentContainerBase(world, detail::generate_index_for_type<T>(world)),
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
			ComponentId id = _components.insert(y_fwd(args)...);
			set_parent(id, parent);
			return id;
		}

		void flush() override {
			for(ComponentId id : _deletions) {
				_components.erase(id);
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



		// ------------------------------------- serde BS -------------------------------------

		static constexpr bool is_serde_compatible = (serde::is_serializable<T>::value && serde::is_deserializable<T>::value) ||
													std::is_trivially_copyable_v<T>;

		u64 serialization_type_id() const override {
			return _registerer->type.type_id();
		}

		void serialize(io::WriterRef writer) const override {
			if constexpr(is_serde_compatible) {
				serde::serialize(writer, _parents);
				serde::serialize(writer, _components);
			}
		}

		void deserialize(io::ReaderRef reader) {
			if constexpr(is_serde_compatible) {
				serde::deserialize(reader, _parents);
				serde::deserialize(reader, _components);
				// fixup references
				for(auto p : _components.as_pairs()) {
					set_parent(p.first, parent(p.first));
				}
			}
		}

		static std::unique_ptr<ComponentContainerBase> deserialized(io::ReaderRef reader, EntityWorld& world) {
			try {
				auto container = std::make_unique<ComponentContainer<T>>(world);
				container->deserialize(reader);
				return container;
			} catch(...) {
			}
			return nullptr;
		}

		static struct Registerer {
			Registerer() {
				detail::register_container_type<T>(&type, is_serde_compatible ? &deserialized : nullptr);
			}

			detail::RegisteredContainerType type;
		} registerer;

		// make sure that it is ODR used no matter what
		Registerer* _registerer = nullptr;
};


template<typename T>
typename ComponentContainer<T>::Registerer ComponentContainer<T>::registerer = ComponentContainer<T>::Registerer();

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

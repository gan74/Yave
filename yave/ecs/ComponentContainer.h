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
#include "EntityIdPool.h"

#include <y/core/SparseVector.h>

#include <y/serde/serde.h>

namespace yave {
namespace ecs {

class ComponentContainerBase;

namespace detail {
using create_container_t = std::unique_ptr<ComponentContainerBase> (*)(io::ReaderRef);
class RegisteredContainerType {
	public:
		u64 type_id() const {
			return _type_id;
		}
	private:
		friend void register_container_type(RegisteredContainerType*, usize, create_container_t);
		friend usize registered_types_count();
		friend void serialize_container(io::WriterRef, ComponentContainerBase*);
		friend std::unique_ptr<ComponentContainerBase> deserialize_container(io::ReaderRef);

		u64 _type_id = 0;
		create_container_t _create_container = nullptr;
		RegisteredContainerType* _next = nullptr;
};

usize registered_types_count();
void register_container_type(RegisteredContainerType* type, u64 type_id, create_container_t create_container);
void serialize_container(io::WriterRef writer, ComponentContainerBase* container);
std::unique_ptr<ComponentContainerBase> deserialize_container(io::ReaderRef reader);

template<typename T>
void register_container_type(RegisteredContainerType* type, create_container_t create_container) {
	// type_hash is not portable, but without reflection we don't have a choice...
	register_container_type(type, type_hash<T>(), create_container);
}
}


class ComponentContainerBase : NonMovable {
	public:
		using index_type = typename EntityId::index_type;

		virtual ~ComponentContainerBase() {
		}

		virtual void remove(core::ArrayView<EntityId> ids) = 0;
		virtual bool has(EntityId id) const = 0;
		virtual core::Span<index_type> indexes() const = 0;

		ComponentTypeIndex type() const {
			return _type;
		}

		template<typename T, typename... Args>
		T& create(EntityId id, Args&&... args) {
			auto i = id.index();
			return component_vector_fast<T>().insert(i, y_fwd(args)...);
		}

		template<typename T, typename... Args>
		T& create_or_find(EntityId id, Args&&... args) {
			auto i = id.index();
			auto& vec = component_vector_fast<T>();
			if(!vec.has(i)) {
				return vec.insert(i, y_fwd(args)...);
			}
			return vec[i];
		}

		template<typename T>
		T& get(EntityId id) {
			return component_vector_fast<T>()[id.index()];
		}

		template<typename T>
		const T& get(EntityId id) const {
			return component_vector_fast<T>()[id.index()];
		}

		template<typename T>
		bool has(EntityId id) const {
			return component_vector_fast<T>().has(id.index());
		}

		template<typename T>
		T& component(EntityId id) {
			return component_vector_fast<T>()[id.index()];
		}

		template<typename T>
		T& component(EntityId id) const {
			return component_vector_fast<T>()[id.index()];
		}

		template<typename T>
		core::MutableSpan<T> components() {
			return component_vector_fast<T>().values();
		}

		template<typename T>
		core::Span<T> components() const {
			return component_vector_fast<T>().values();
		}

		template<typename T>
		auto& component_vector() {
			return component_vector_fast<T>();
		}

		template<typename T>
		const auto& component_vector() const {
			return component_vector_fast<T>();
		}

	protected:
		template<typename T>
		ComponentContainerBase(core::SparseVector<T, index_type>& sparse) :
				_sparse_ptr(&sparse),
				_type(index_for_type<T>()) {
		}



		template<typename T>
		auto& component_vector_fast() {
			y_debug_assert(type() == index_for_type<T>());
			return (*static_cast<core::SparseVector<T, index_type>*>(_sparse_ptr));
		}

		template<typename T>
		const auto& component_vector_fast() const {
			y_debug_assert(type() == index_for_type<T>());
			return (*static_cast<const core::SparseVector<T, index_type>*>(_sparse_ptr));
		}

	private:
		// hacky but avoids dynamic casts and virtual calls
		void* _sparse_ptr = nullptr;
		const ComponentTypeIndex _type;


	private:
		friend void detail::serialize_container(io::WriterRef, ComponentContainerBase*);

		virtual void serialize(io::WriterRef) const = 0;
		virtual u64 serialization_type_id() const = 0;

};




template<typename T>
class ComponentContainer final : public ComponentContainerBase {
	public:
		ComponentContainer() :
				ComponentContainerBase(_components), // we don't actually need to care about initilisation order here
				_registerer(&registerer) {
		}

		void remove(core::Span<EntityId> ids) override {
			for(EntityId id : ids) {
				auto i = id.index();
				if(_components.has(i)) {
					_components.erase(i);
				}
			}
		}

		bool has(EntityId id) const override {
			return ComponentContainerBase::has<T>(id);
		}

		core::Span<index_type> indexes() const override {
			return _components.indexes();
		}

	private:
		core::SparseVector<T, index_type> _components;



	// ------------------------------------- serde BS -------------------------------------

	private:
		static constexpr bool is_serde_compatible = (serde::is_serializable<T>::value && serde::is_deserializable<T>::value) ||
													std::is_trivially_copyable_v<T>;

		u64 serialization_type_id() const override {
			return _registerer->type.type_id();
		}

		void serialize(io::WriterRef writer) const override {
			if constexpr(is_serde_compatible) {
				writer->write_one(u64(_components.size()));
				for(auto p : _components.as_pairs()) {
					writer->write_one(u64(p.first));
					serde::serialize(writer, p.second);
				}
			}
		}

		void deserialize(io::ReaderRef reader) {
			if constexpr(is_serde_compatible) {
				u64 component_count = reader->read_one<u64>();
				for(u64 i = 0; i != component_count; ++i) {
					index_type id = index_type(reader->read_one<u64>());
					serde::deserialize(reader, _components.insert(id));
				}
			}
		}

		static std::unique_ptr<ComponentContainerBase> deserialized(io::ReaderRef reader) {
			try {
				auto container = std::make_unique<ComponentContainer<T>>();
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

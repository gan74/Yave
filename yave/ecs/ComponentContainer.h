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
#include "EntityId.h"

#include <yave/assets/AssetArchive.h>
#include <y/core/SparseVector.h>


namespace yave {
namespace ecs {

template<typename T>
using ComponentVector = core::SparseVector<T, EntityIndex>;


class ComponentContainerBase;

namespace detail {
using create_container_t = std::unique_ptr<ComponentContainerBase> (*)();
struct RegisteredContainerType {
	u64 type_id = 0;
	std::type_index type_index = typeid(void);
	create_container_t create_container = nullptr;
	RegisteredContainerType* next = nullptr;
};

usize registered_types_count();
void register_container_type(RegisteredContainerType* type, u64 type_id, std::type_index type_index, create_container_t create_container);
serde2::Result serialize_container(WritableAssetArchive& writer, ComponentContainerBase* container);
std::unique_ptr<ComponentContainerBase> deserialize_container(ReadableAssetArchive& reader);
std::unique_ptr<ComponentContainerBase> create_container(std::type_index type_index);

template<typename T>
void register_container_type(RegisteredContainerType* type, create_container_t create_container) {
	// type_hash is not portable, but without reflection we don't have a choice...
	register_container_type(type, type_hash<T>(), typeid(T), create_container);
}
}


class ComponentContainerBase : NonMovable {
	public:
		virtual ~ComponentContainerBase() {
		}

		virtual void remove(core::ArrayView<EntityId> ids) = 0;
		virtual bool has(EntityId id) const = 0;
		virtual core::Result<void> create_empty(EntityId id) = 0;
		virtual core::Span<EntityIndex> indexes() const = 0;

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
		T& component(EntityId id) {
			return component_vector_fast<T>()[id.index()];
		}

		template<typename T>
		const T& component(EntityId id) const {
			return component_vector_fast<T>()[id.index()];
		}

		template<typename T>
		T* component_ptr(EntityId id) {
			return component_vector_fast<T>().try_get(id.index());
		}

		template<typename T>
		const T* component_ptr(EntityId id) const {
			return component_vector_fast<T>().try_get(id.index());
		}


		template<typename T>
		bool has(EntityId id) const {
			return component_vector_fast<T>().has(id.index());
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
		ComponentContainerBase(ComponentVector<T>& sparse) :
				_sparse_ptr(&sparse),
				_type(index_for_type<T>()) {
		}



		template<typename T>
		auto& component_vector_fast() {
			y_debug_assert(type() == index_for_type<T>());
			return (*static_cast<ComponentVector<T>*>(_sparse_ptr));
		}

		template<typename T>
		const auto& component_vector_fast() const {
			y_debug_assert(type() == index_for_type<T>());
			return (*static_cast<const ComponentVector<T>*>(_sparse_ptr));
		}

	private:
		// hacky but avoids dynamic casts and virtual calls
		void* _sparse_ptr = nullptr;
		const ComponentTypeIndex _type;


	private:
		friend serde2::Result detail::serialize_container(WritableAssetArchive&, ComponentContainerBase*);
		friend std::unique_ptr<ComponentContainerBase> detail::deserialize_container(ReadableAssetArchive&);

		virtual serde2::Result serialize(WritableAssetArchive&) const = 0;
		virtual serde2::Result deserialize(ReadableAssetArchive&) = 0;
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

		core::Result<void> create_empty(EntityId id) override {
			if constexpr(std::is_default_constructible_v<T>) {
				ComponentContainerBase::create<T>(id);
				return core::Ok();
			}
			unused(id);
			return core::Err();
		}

		core::Span<EntityIndex> indexes() const override {
			return _components.indexes();
		}

	private:
		ComponentVector<T> _components;



	// ------------------------------------- serde BS -------------------------------------

	private:
		static constexpr bool is_serde_compatible = serde2::is_serializable<WritableAssetArchive, T>::value && serde2::is_deserializable<ReadableAssetArchive, T>::value;

		u64 serialization_type_id() const override {
			return _registerer->type.type_id;
		}

		serde2::Result serialize(WritableAssetArchive& writer) const override {
			if constexpr(is_serde_compatible) {
				if(!writer(u64(_components.size()))) {
					return core::Err();
				}
				for(auto p : _components.as_pairs()) {
					if(!writer(u64(p.first)) || !writer(p.second)) {
						return core::Err();
					}
				}
			}
			return core::Ok();
		}

		serde2::Result deserialize(ReadableAssetArchive& reader) override {
			if constexpr(is_serde_compatible) {
				u64 component_count = 0;
				if(!reader(component_count)) {
					return core::Err();
				}
				for(u64 i = 0; i != component_count; ++i) {
					u64 id = 0;
					if(!reader(id) || !reader(_components.insert(EntityIndex(id)))) {
						return core::Err();
					}
				}
				y_debug_assert(_components.size() == component_count);
			}
			return core::Ok();
		}

		static struct Registerer {
			Registerer() {
				detail::register_container_type<T>(&type, []() -> std::unique_ptr<ComponentContainerBase> {
						return std::make_unique<ComponentContainer<T>>();
					});
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

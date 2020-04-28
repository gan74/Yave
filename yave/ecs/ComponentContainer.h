/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <yave/utils/serde.h>

#include <y/serde3/archives.h>
#include <y/core/SparseVector.h>
#include <y/core/Span.h>
#include <y/core/Result.h>

#include <unordered_map>

namespace yave {
namespace ecs {

template<typename T>
using ComponentVector = core::SparseVector<T, EntityIndex>;

class ComponentContainerBase;

namespace detail {
template<typename T>
using has_required_components_t = decltype(std::declval<T>().required_components_archetype());
}


class ComponentContainerBase : NonMovable {
	public:
		virtual ~ComponentContainerBase();

		virtual void remove(core::Span<EntityId> ids) = 0;
		virtual bool has(EntityId id) const = 0;
		virtual core::Result<void> create_one(EntityWorld& world, EntityId id) = 0;
		virtual core::Span<EntityIndex> indexes() const = 0;

		virtual std::string_view component_type_name() const = 0;

		ComponentTypeIndex type() const {
			return _type;
		}

		template<typename T, typename... Args>
		T& create(EntityWorld& world, EntityId id, Args&&... args) {
			auto i = id.index();
			auto& vec = component_vector_fast<T>();
			add_required_components<T>(world, id);
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
		ComponentVector<T>& component_vector() {
			return component_vector_fast<T>();
		}

		template<typename T>
		const ComponentVector<T>& component_vector() const {
			return component_vector_fast<T>();
		}


		y_serde3_poly_base(ComponentContainerBase)
		virtual void post_deserialize_poly(AssetLoadingContext&) = 0;

	protected:
		template<typename T>
		ComponentContainerBase(ComponentVector<T>& sparse) :
				_sparse_ptr(&sparse),
				_type(index_for_type<T>()) {
		}

		template<typename T>
		static void add_required_components(EntityWorld& world, EntityId id) {
			unused(world, id);
			if constexpr(is_detected_v<detail::has_required_components_t, T>) {
				T::add_required_components(world, id);
			}
		}

	private:
		// hacky but avoids dynamic casts and virtual calls
		void* _sparse_ptr = nullptr;
		const ComponentTypeIndex _type;


		template<typename T>
		auto& component_vector_fast() {
			y_debug_assert(_sparse_ptr);
			y_debug_assert(type() == index_for_type<T>());
			return (*static_cast<ComponentVector<T>*>(_sparse_ptr));
		}

		template<typename T>
		const auto& component_vector_fast() const {
			y_debug_assert(_sparse_ptr);
			y_debug_assert(type() == index_for_type<T>());
			return (*static_cast<const ComponentVector<T>*>(_sparse_ptr));
		}
};




template<typename T>
class ComponentContainer final : public ComponentContainerBase {
	public:
		ComponentContainer() : ComponentContainerBase(_components) {
		}

		void remove(core::Span<EntityId> ids) override {
			y_profile();
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

		core::Result<void> create_one(EntityWorld& world, EntityId id) override {
			if constexpr(std::is_default_constructible_v<T>) {
				ComponentContainerBase::create<T>(world, id);
				return core::Ok();
			}
			unused(id);
			return core::Err();
		}

		core::Span<EntityIndex> indexes() const override {
			return _components.indexes();
		}

		std::string_view component_type_name() const override {
			return ct_type_name<T>();
		}

		y_serde3(_components)
		y_serde3_poly(ComponentContainer)

		void post_deserialize_poly(AssetLoadingContext& context) override {
			y_profile();
			serde3::ReadableArchive::post_deserialize(*this, context);
		}

	private:
		ComponentVector<T> _components;
};

static_assert(serde3::has_serde3_poly_v<ComponentContainerBase>);
static_assert(serde3::has_serde3_ptr_poly_v<ComponentContainerBase*>);
static_assert(serde3::has_serde3_post_deser_poly_v<ComponentContainerBase, AssetLoadingContext&>);

}
}

#endif // YAVE_ECS_COMPONENTCONTAINER_H

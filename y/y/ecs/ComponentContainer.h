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
#ifndef Y_ECS_COMPONENTCONTAINER_H
#define Y_ECS_COMPONENTCONTAINER_H

#include "ecs.h"
#include "SparseComponentSet.h"
#include "ComponentRuntimeInfo.h"

#include <y/serde3/archives.h>

namespace y {
namespace ecs {

namespace detail {
template<typename T>
using has_required_components_t = decltype(std::declval<T>().required_components_archetype());
}



class ComponentBoxBase : NonMovable {
	public:
		virtual ~ComponentBoxBase();

		virtual ComponentRuntimeInfo runtime_info() const = 0;
		virtual void add_to(EntityID id, ComponentContainerBase* container) const = 0;

		y_serde3_poly_base(ComponentBoxBase)
};

template<typename T>
class ComponentBox final : public ComponentBoxBase {
	public:
		static_assert(std::is_copy_constructible_v<T>);

		ComponentBox(const T& t);

		ComponentRuntimeInfo runtime_info() const override;
		void add_to(EntityID id, ComponentContainerBase* container) const override;

		y_serde3(_component)
		y_serde3_poly(ComponentBox)

	private:
		T _component;
};



class ComponentContainerBase : NonMovable {
	public:
		virtual ~ComponentContainerBase();

		EntityWorld& world();

		bool contains(EntityID id) const;
		core::Span<EntityID> ids() const;

		ComponentTypeIndex type_id() const;


		virtual std::string_view component_type_name() const = 0;
		virtual ComponentRuntimeInfo runtime_info() const = 0;

		virtual void add(EntityID id) = 0;
		virtual void remove(EntityID id) = 0;

		virtual std::unique_ptr<ComponentBoxBase> create_box(EntityID id) const = 0;


		template<typename T, typename... Args>
		T& add(EntityID id, Args&&... args) {
			auto& set = component_set_fast<T>();
			if(!set.contains_index(id.index())) {
				add_required_components<T>(id);
				return set.insert(id, y_fwd(args)...);
			}
			return set[id];
		}

		template<typename T>
		T& component(EntityID id) {
			return component_set_fast<T>()[id];
		}

		template<typename T>
		const T& component(EntityID id) const {
			return component_set_fast<T>()[id];
		}

		template<typename T>
		T* component_ptr(EntityID id) {
			return component_set_fast<T>().try_get(id);
		}

		template<typename T>
		const T* component_ptr(EntityID id) const {
			return component_set_fast<T>().try_get(id);
		}

		template<typename T>
		core::MutableSpan<T> components() {
			return component_set_fast<T>().values();
		}

		template<typename T>
		core::Span<T> components() const {
			return component_set_fast<T>().values();
		}

		template<typename T>
		SparseComponentSet<T>& component_set() {
			return component_set_fast<T>();
		}

		template<typename T>
		const SparseComponentSet<T>& component_set() const {
			return component_set_fast<T>();
		}




		y_serde3_poly_base(ComponentContainerBase)

	protected:
		template<typename T>
		ComponentContainerBase(SparseComponentSet<T>& sparse, EntityWorld* world) :
				_sparse(&sparse),
				_ids(&sparse),
				_world(world),
				_type_id(type_index<T>()) {

			y_debug_assert(world);
		}


		template<typename T>
		void add_required_components(EntityID id) {
			unused(id);
			if constexpr(is_detected_v<detail::has_required_components_t, T>) {
				T::add_required_components(*_world, id);
			}
		}

	private:
		// hacky but avoids a bunch of dynamic casts and virtual calls
		void* _sparse = nullptr;
		SparseIDSet* _ids = nullptr;
		EntityWorld* _world = nullptr;

		const ComponentTypeIndex _type_id;


		template<typename T>
		auto& component_set_fast() {
			y_debug_assert(_sparse);
			y_debug_assert(type_index<T>() == _type_id);
			return *static_cast<SparseComponentSet<T>*>(_sparse);
		}

		template<typename T>
		const auto& component_set_fast() const {
			y_debug_assert(_sparse);
			y_debug_assert(type_index<T>() == _type_id);
			return *static_cast<const SparseComponentSet<T>*>(_sparse);
		}
};


template<typename T>
class ComponentContainer final : public ComponentContainerBase {
	public:
		ComponentContainer(EntityWorld* world) : ComponentContainerBase(_components, world) {
		}

		std::string_view component_type_name() const override {
			return ct_type_name<T>();
		}

		ComponentRuntimeInfo runtime_info() const override {
			return ComponentRuntimeInfo::create<T>();
		}

		void add(EntityID id) override {
			ComponentContainerBase::add<T>(id);
		}

		void remove(EntityID id) override {
			if(_components.contains(id)) {
				_components.erase(id);
			}
		}

		std::unique_ptr<ComponentBoxBase> create_box(EntityID id) const override {
			unused(id);
			if constexpr(std::is_copy_constructible_v<T>) {
				return std::make_unique<ComponentBox<T>>(_components[id]);
			}
			return nullptr;
		}

		y_serde3(_components)
		y_serde3_poly(ComponentContainer)

	private:
		SparseComponentSet<T> _components;
};






template<typename T>
ComponentBox<T>::ComponentBox(const T& t) : _component(t) {
}

template<typename T>
ComponentRuntimeInfo ComponentBox<T>::runtime_info() const {
	return ComponentRuntimeInfo::create<T>();
}

template<typename T>
void ComponentBox<T>::add_to(EntityID id, ComponentContainerBase* container) const {
	container->add<T>(id, _component);
}

template<typename T>
std::unique_ptr<ComponentContainerBase> create_container(EntityWorld* world) {
	return std::make_unique<ComponentContainer<T>>(world);
}

}
}

#endif // Y_ECS_COMPONENTCONTAINER_H

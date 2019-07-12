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
#ifndef YAVE_ECS_ENTITYWORLD_H
#define YAVE_ECS_ENTITYWORLD_H

#include <yave/utils/serde.h>

#include "ComponentContainer.h"
#include "EntityIdPool.h"
#include "View.h"

#include <yave/assets/AssetType.h>

#include <y/core/Result.h>

#include <unordered_map>

namespace yave {
namespace ecs {

class EntityWorld : NonCopyable {

	Y_TODO(this should be in utils somewhere)
	class ComponentTypeIterator {
		using iterator_type = std::unordered_map<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>>::const_iterator;
		public:
			ComponentTypeIterator(iterator_type it) : _it(it) {
			}

			ComponentTypeIterator& operator++() {
				++_it;
				return *this;
			}

			ComponentTypeIterator operator++(int) {
				iterator_type it = _it;
				++_it;
				return ComponentTypeIterator(it);
			}

			bool operator==(const ComponentTypeIterator& other) const {
				return _it == other._it;
			}

			bool operator!=(const ComponentTypeIterator& other) const {
				return _it != other._it;
			}

			const ComponentTypeIndex& operator*() const {
				return _it->first;
			}

			const ComponentTypeIndex* operator->() const {
				return &_it->first;
			}

		private:
			iterator_type _it;
	};

	public:
		template<typename... Args>
		using EntityView = View<false, Args...>;
		template<typename... Args>
		using ConstEntityView = View<true, Args...>;

		EntityWorld();

		EntityId create_entity();
		void remove_entity(EntityId id);

		EntityId id_from_index(EntityIndex index) const;

		bool exists(EntityId id) const;

		const EntityIdPool& entities() const;

		void flush();


		void add(const EntityWorld& other);

		serde2::Result serialize(WritableAssetArchive& writer) const;
		serde2::Result deserialize(ReadableAssetArchive& reader);



		core::Result<void> create_component(EntityId id, ComponentTypeIndex type) {
			if(ComponentContainerBase* cont = container(type)) {
				return cont->create_empty(id);
			}
			return core::Err();
		}

		template<typename T, typename... Args>
		T& create_component(EntityId id, Args&&... args) {
			return container<T>()->template create<T>(id, y_fwd(args)...);
		}

		template<typename T, typename... Args>
		T& create_or_find_component(EntityId id, Args&&... args) {
			return container<T>()->template create_or_find<T>(id, y_fwd(args)...);
		}

		template<typename T, typename... Args>
		auto create_components(EntityId id) {
			create_component<T>(id);
			if constexpr(sizeof...(Args)) {
				create_components<Args...>(id);
			}
		}



		template<typename T>
		T* component(EntityId id) {
			return container<T>()->template component_ptr<T>(id);
		}

		template<typename T>
		const T* component(EntityId id) const {
			return container<T>()->template component_ptr<T>(id);
		}

		/*template<typename T>
		T& operator[](EntityId id) {
			return container<T>()->template component<T>(id);
		}

		template<typename T>
		const T& operator[](EntityId id) const {
			return container<T>()->template component<T>(id);
		}*/


		template<typename T>
		bool has(EntityId id) const {
			if(!exists(id) ) {
				return false;
			}
			const ComponentContainerBase* cont = container<T>();
			return cont ? cont->has<T>(id) : false;
		}

		bool has(EntityId id, ComponentTypeIndex type) const {
			if(!exists(id) ) {
				return false;
			}
			const ComponentContainerBase* cont = container(type);
			return cont ? cont->has(id) : false;
		}



		template<typename T>
		core::MutableSpan<T> components() {
			return container<T>()->template components<T>();
		}

		template<typename T>
		core::Span<T> components() const {
			const ComponentContainerBase* cont = container<T>();
			return cont ? cont->components<T>() : decltype(cont->components<T>())();
		}


		template<typename... Args>
		EntityView<Args...> view() {
			static_assert(sizeof...(Args));
			return EntityView<Args...>(typed_component_vectors<Args...>());
		}

		template<typename... Args>
		ConstEntityView<Args...> view() const {
			static_assert(sizeof...(Args));
			return ConstEntityView<Args...>(typed_component_vectors<Args...>());
		}


		template<typename T>
		core::Span<EntityIndex> indexes() const {
			return indexes(index_for_type<T>());
		}

		core::Span<EntityIndex> indexes(ComponentTypeIndex type) const {
			const ComponentContainerBase* cont = container(type);
			return cont ? cont->indexes() : core::Span<EntityIndex>();
		}






		template<typename... Args>
		EntityId create_entity(EntityArchetype<Args...>) {
			EntityId ent = create_entity();
			create_components<Args...>(ent);
			return ent;
		}

		template<typename... Args>
		EntityView<Args...> view(EntityArchetype<Args...>) {
			return view<Args...>();
		}

		template<typename... Args>
		ConstEntityView<Args...> view(EntityArchetype<Args...>) const {
			return view<Args...>();
		}


		usize component_type_count() const {
			return _component_containers.size();
		}

		auto component_types() const {
			return core::Range(ComponentTypeIterator(_component_containers.begin()),
							   ComponentTypeIterator(_component_containers.end()));
		}



		core::String type_name(ComponentTypeIndex type) const;

	private:
		template<typename T>
		ComponentContainerBase* container() {
			auto& container = _component_containers[index_for_type<T>()];
			if(!container) {
				container = std::make_unique<ComponentContainer<T>>();
			}
			return container.get();
		}

		template<typename T>
		const ComponentContainerBase* container() const {
			return container(index_for_type<T>());
		}


		template<typename T, typename... Args>
		std::tuple<ComponentVector<T>*, ComponentVector<Args>*...> typed_component_vectors() const {
			if constexpr(sizeof...(Args)) {
				return std::tuple_cat(typed_component_vectors<T>(),
									  typed_component_vectors<Args...>());
			} else {
				// We need non consts here and we want to avoir returning non const everywhere else
				// This shouldn't be UB as component containers are never const
				ComponentContainerBase* cont = const_cast<ComponentContainerBase*>(container<T>());
				return cont ? &cont->component_vector<T>() : nullptr;
			}
		}


		const ComponentContainerBase* container(ComponentTypeIndex type) const;
		ComponentContainerBase* container(ComponentTypeIndex type);

		EntityIdPool _entities;
		core::Vector<EntityId> _deletions;

		std::unordered_map<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>> _component_containers;
};

}
}


#endif // YAVE_ECS_ENTITYWORLD_H

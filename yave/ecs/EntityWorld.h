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
#ifndef YAVE_ECS_ENTITYWORLD_H
#define YAVE_ECS_ENTITYWORLD_H

#include <yave/utils/serde.h>

#include "ComponentContainer.h"
#include "EntityIdPool.h"
#include "View.h"

#include <yave/assets/AssetType.h>

#include <y/core/Result.h>
#include <y/utils/iter.h>

#include <unordered_map>

namespace yave {
namespace ecs {

class EntityWorld : NonCopyable {

	using ComponentTypeIterator = TupleMemberIterator<0, std::unordered_map<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>>::const_iterator>;

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


		template<typename T>
		void add_required_component_type() {
			static_assert(std::is_default_constructible_v<T>);
			_required_components << index_for_type<T>();
			for(EntityId id : entities()) {
				create_component<T>(id);
			}
		}


		core::Result<void> create_component(EntityId id, ComponentTypeIndex type) {
			y_debug_assert(exists(id));
			if(ComponentContainerBase* cont = container(type)) {
				return cont->create_one(*this, id);
			}
			return core::Err();
		}

		template<typename T, typename... Args>
		T& create_component(EntityId id, Args&&... args) {
			y_debug_assert(exists(id));
			return container<T>()->template create<T>(*this, id, y_fwd(args)...);
		}

		template<typename T, typename... Args>
		void create_components(EntityId id) {
			y_debug_assert(exists(id));
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


		template<typename T>
		bool has(EntityId id) const {
			if(!exists(id)) {
				return false;
			}
			const ComponentContainerBase* cont = container<T>();
			return cont ? cont->has<T>(id) : false;
		}

		bool has(EntityId id, ComponentTypeIndex type) const {
			if(!exists(id)) {
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

		core::Span<ComponentTypeIndex> required_component_types() const {
			return _required_components;
		}


		std::string_view component_type_name(ComponentTypeIndex type) const;


		void flush_reload(AssetLoader& loader);

		y_serde3(_entities, _component_containers)

	private:
		struct Hash {
			usize operator()(ComponentTypeIndex index) const {
				return usize(index.type_hash);
			}
		};


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

		void add_required_components(EntityId id);

		EntityIdPool _entities;
		core::Vector<EntityId> _deletions;

		std::unordered_map<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>,Hash> _component_containers;

		Y_TODO(Do we have to serialize this?)
		core::Vector<ComponentTypeIndex> _required_components;
};



template<typename... Args>
void RequiredComponents<Args...>::add_required_components(EntityWorld& world, EntityId id) {
	world.create_components<Args...>(id);
}

}
}


#endif // YAVE_ECS_ENTITYWORLD_H

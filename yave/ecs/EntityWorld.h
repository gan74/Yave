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

#include "ComponentContainer.h"
#include "EntityIdPool.h"
#include "View.h"

#include <y/core/Result.h>

#include <unordered_map>

namespace yave {
namespace ecs {

class EntityWorld : NonCopyable {

	using index_type = typename EntityId::index_type;

	public:
		template<typename... Args>
		using EntityView = View<false, index_type, Args...>;
		template<typename... Args>
		using ConstEntityView = View<true, index_type, Args...>;

		EntityWorld();

		EntityId create_entity();
		void remove_entity(EntityId id);

		const EntityIdPool& entities() const;

		void flush();




		void serialize(io::WriterRef writer) const;
		void deserialize(io::ReaderRef reader);



		template<typename T, typename... Args>
		T& create_component(EntityId id, Args&&... args) {
			return container<T>()->template create<T>(id, y_fwd(args)...);
		}

		template<typename T, typename... Args>
		T& create_or_find_component(EntityId id, Args&&... args) {
			return container<T>()->template create_or_find<T>(id, y_fwd(args)...);
		}


		template<typename T>
		T& component(EntityId id) {
			return container<T>()->template component<T>(id);
		}

		template<typename T>
		const T& component(EntityId id) const {
			return container<T>()->template component<T>(id);
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
			return typed_component_vectors<Args...>();
		}

		template<typename... Args>
		ConstEntityView<Args...> view() const {
			static_assert(sizeof...(Args));
			return typed_component_vectors<Args...>();
		}


		template<typename T>
		core::Span<index_type> indexes() const {
			return indexes(index_for_type<T>());
		}

		core::Span<index_type> indexes(ComponentTypeIndex type) const {
			const ComponentContainerBase* cont = container(type);
			return cont ? cont->indexes() : core::Span<index_type>();
		}



		const auto& component_containers() const {
			return _component_containers;
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

		const ComponentContainerBase* container(ComponentTypeIndex type) const {
			if(auto it = _component_containers.find(type); it != _component_containers.end()) {
				return it->second.get();
			}
			return nullptr;
		}


		template<typename T, typename... Args>
		std::tuple<core::SparseVector<T, index_type>&, core::SparseVector<Args, index_type>&...> typed_component_vectors() const {
			if constexpr(sizeof...(Args)) {
				return std::tuple_cat(typed_component_vectors<T>(),
									  typed_component_vectors<Args...>());
			} else {
				// We need non consts here and we want to avoir returning non const everywhere else
				// Const safety for typed_component_containers is done in ReturnComponentsPolicy
				// This shouldn't be UB as component containers are never const
				ComponentContainerBase* cont = const_cast<ComponentContainerBase*>(container<T>());
				return std::make_tuple(cont->component_vector<T>());
			}
		}



		EntityIdPool _entities;
		core::Vector<EntityId> _deletions;

		std::unordered_map<ComponentTypeIndex, std::unique_ptr<ComponentContainerBase>> _component_containers;
};

}
}


#endif // YAVE_ECS_ENTITYWORLD_H

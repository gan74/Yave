/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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
#ifndef Y_ECS_ENTITYWORLD_H
#define Y_ECS_ENTITYWORLD_H

#include "EntityView.h"

#include <y/utils/sort.h>
#include <y/utils/iter.h>

namespace y {
namespace ecs {

class ComponentSerializerBase;
class EntityWorldSerializer;

class EntityWorld : NonCopyable {
	public:
		bool exists(EntityID id) const;

		EntityID create_entity();
		void remove_entity(EntityID id);

		core::Span<std::unique_ptr<Archetype>> archetypes() const;



		template<typename... Args>
		EntityView<Args...> view() {
			return EntityView<Args...>(_archetypes);
		}

		auto entity_ids() const {
			auto ids = TransformIterator(_entities.begin(), [](const EntityData& data) { return data.id; });
			return core::Range(FilterIterator(ids, _entities.end(), [](EntityID id) { return id.is_valid(); }), EndIterator());
		}



		template<typename T>
		T* component(EntityID id) {
			return find_component<T>(id);
		}

		template<typename T>
		const T* component(EntityID id) const {
			return find_component<T>(id);
		}



		template<typename T>
		void add_component(EntityID id) {
			add_components<T>(id);
		}

		template<typename... Args>
		void add_components(EntityID id) {
			check_exists(id);

			EntityData& data = _entities[id.index()];
			Archetype* old_arc = data.archetype;
			Archetype* new_arc = nullptr;

			core::Vector types = core::vector_with_capacity<u32>((old_arc ? old_arc->component_count() : 0) + sizeof...(Args));
			{
				{
					if(old_arc) {
						for(const ComponentRuntimeInfo& info : old_arc->component_infos()) {
							types << info.type_id;
						}
					}
					add_type_indexes<0, Args...>(types);
					sort(types.begin(), types.end());
				}

				for(const auto& arc : _archetypes) {
					if(arc->matches_type_indexes(types)) {
						new_arc = arc.get();
						break;
					}
				}

				if(!new_arc) {
					if(old_arc) {
						new_arc = _archetypes.emplace_back(old_arc->archetype_with<Args...>()).get();
					} else {
						new_arc = _archetypes.emplace_back(Archetype::create<Args...>()).get();
					}
				}
			}
			y_debug_assert(new_arc->_component_count == types.size());
			transfer(data, new_arc);
		}


	private:
		friend class ComponentSerializerBase;
		friend class EntityWorldSerializer;

		void check_exists(EntityID id) const;

		void transfer(EntityData& data, Archetype* to);


		template<usize I, typename... Args>
		static void add_type_indexes(core::Vector<u32>& types) {
			static_assert(sizeof...(Args));
			if constexpr(I < sizeof...(Args)) {
				using type = std::tuple_element_t<I, std::tuple<Args...>>;
				types << type_index<type>();
				add_type_indexes<I + 1, Args...>(types);
			}
		}

		// Not const correct, do not expose publicly
		template<typename T>
		T* find_component(EntityID id) const {
			if(!exists(id)) {
				return nullptr;
			}
			const EntityData& data =_entities[id.index()];
			y_debug_assert(data.id == id);
			if(!data.archetype) {
				return nullptr;
			}
			const auto& view = data.archetype->view<T>();
			if(view.is_empty()) {
				return nullptr;
			}
			return &std::get<0>(view[data.archetype_index]);
		}



		core::Vector<EntityData> _entities;
		core::Vector<std::unique_ptr<Archetype>> _archetypes;
};

}
}

#endif // Y_ECS_ENTITYWORLD_H

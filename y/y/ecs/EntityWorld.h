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

class EntityWorld : NonCopyable {

	using CallbackFunc = std::function<void(const EntityWorld&, EntityID)>;

	struct ComponentCallBacks {
		core::Vector<CallbackFunc> on_create;
		core::Vector<CallbackFunc> on_remove;
	};

	public:
		bool exists(EntityID id) const;

		EntityID create_entity();
		EntityID create_entity(const ArchetypeRuntimeInfo& archetype);

		void remove_entity(EntityID id);

		const Archetype* archetype(EntityID id) const;

		core::Span<std::unique_ptr<Archetype>> archetypes() const;


		template<typename... Args>
		EntityView<Args...> view() {
			return EntityView<Args...>(EntityIterator<Args...>(_archetypes));
		}

		template<typename... Args>
		EntityView<Args...> view(StaticArchetype<Args...>) {
			return view<Args...>();
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

			Y_TODO(maybe replace with find_or_create_archetype, is it slower?)
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

					for_each_type_index<0, Args...>([&](u32 type_id) { types.emplace_back(type_id); });
					sort(types.begin(), types.end());
				}

				for(const auto& arc : _archetypes) {
					if(arc->_info.matches_type_indexes(types)) {
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

			y_debug_assert(new_arc->component_count() == types.size());
			transfer(data, new_arc);

			for_each_type_index<0, Args...>([&](u32 type_id) {
				on_create(type_id, id);
			});
		}

		template<typename... Args>
		void add_components(EntityID id, StaticArchetype<Args...>) {
			add_components<Args...>(id);
		}


		template<typename T>
		void remove_component(EntityID id) {
			remove_components<T>(id);
		}

		template<typename... Args>
		void remove_components(EntityID id) {
			check_exists(id);

			EntityData& data = _entities[id.index()];
			Archetype* old_arc = data.archetype;
			if(!old_arc) {
				return;
			}

			Archetype* new_arc = nullptr;
			core::Vector types = core::vector_with_capacity<u32>(old_arc->component_count());
			{
				for(const ComponentRuntimeInfo& info : old_arc->component_infos()) {
					if(!has_type<0, Args...>(info.type_id)) {
						types << info.type_id;
					}
				}

				for(const auto& arc : _archetypes) {
					if(arc->_info.matches_type_indexes(types)) {
						new_arc = arc.get();
						break;
					}
				}

				if(!new_arc) {
					new_arc = add_archetype(old_arc->archetype_with<Args...>());
				}
			}

			y_debug_assert(new_arc->component_count() == types.size());
			transfer(data, new_arc);

			for_each_type_index<0, Args...>([&](u32 type_id) {
				on_remove(type_id, id);
			});
		}

		template<typename T>
		void add_on_create(CallbackFunc func) {
			add_on_create(type_index<T>(), std::move(func));
		}

		template<typename T>
		void add_on_remove(CallbackFunc func) {
			add_on_remove(type_index<T>(), std::move(func));
		}


		y_serde3(_entities, _archetypes)
		void post_deserialize() const;

	private:
		friend class ComponentInfoSerializerBase;
		friend class EntityWorldSerializer;

		void check_exists(EntityID id) const;

		Archetype* find_or_create_archetype(const ArchetypeRuntimeInfo& info);
		Archetype* add_archetype(std::unique_ptr<Archetype> arc);
		void transfer(EntityData& data, Archetype* to);

		void add_on_create(u32 type_id, CallbackFunc func);
		void add_on_remove(u32 type_id, CallbackFunc func);

		const ComponentCallBacks* component_callbacks(u32 type_id) const;
		void on_create(u32 type_id, EntityID id) const;
		void on_remove(u32 type_id, EntityID id) const;


		template<usize I, typename... Args, typename F>
		static void for_each_type_index(F&& func) {
			if constexpr(I < sizeof...(Args)) {
				using type = std::tuple_element_t<I, std::tuple<Args...>>;
				func(type_index<type>());
				for_each_type_index<I + 1, Args...>(func);
			}
		}

		template<usize I, typename... Args>
		static bool has_type(u32 type_id) {
			if constexpr(I < sizeof...(Args)) {
				using type = std::tuple_element_t<I, std::tuple<Args...>>;
				if(type_id == type_index<type>()) {
					return true;
				}
			}
			return false;
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
			const auto view = data.archetype->components<T>();
			if(view.is_empty()) {
				return nullptr;
			}
			return &view[data.archetype_index];
		}



		core::Vector<EntityData> _entities;
		core::Vector<std::unique_ptr<Archetype>> _archetypes;

		Y_TODO(callbacks wont be called when world is destroyed)
		core::Vector<ComponentCallBacks> _component_callbacks;
};

}
}

#endif // Y_ECS_ENTITYWORLD_H

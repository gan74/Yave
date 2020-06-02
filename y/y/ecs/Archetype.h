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
#ifndef Y_ECS_ARCHETYPE_H
#define Y_ECS_ARCHETYPE_H

#include "ComponentView.h"

#include "EntityPrefab.h"


namespace y {
namespace ecs {

class Archetype : NonMovable {
	static memory::PolymorphicAllocatorBase* default_allocator() {
		return memory::global_allocator();
	}

	public:
		template<typename... Args>
		static std::unique_ptr<Archetype> create(memory::PolymorphicAllocatorBase* allocator = default_allocator()) {
			return std::make_unique<Archetype>(ArchetypeRuntimeInfo::create<Args...>(), allocator);
		}

		static std::unique_ptr<Archetype> create(ArchetypeRuntimeInfo info, memory::PolymorphicAllocatorBase* allocator = default_allocator()) {
			return std::make_unique<Archetype>(std::move(info), allocator);
		}

		template<typename... Args>
		std::unique_ptr<Archetype> archetype_with() {
			return std::make_unique<Archetype>(_info.with<Args...>(), _allocator.allocator());
		}


		~Archetype();

		usize entity_count() const;
		usize component_count() const;

		const ArchetypeRuntimeInfo& runtime_info() const;
		core::Span<ComponentRuntimeInfo> component_infos() const;
		const ComponentRuntimeInfo* component_info(u32 type_id) const;

		void add_entity(EntityData& data);
		void add_entities(core::MutableSpan<EntityData> entities);

		EntityPrefab create_prefab(const EntityData& data);

		void* raw_component(const EntityData& data, u32 type_id);



		template<typename T>
		SingleComponentView<T> components() {
			ComponentIterator<true, T> begin;
			if(!build_iterator<0>(begin)) {
				return SingleComponentView<T>();
			}
			return SingleComponentView<T>(begin, entity_count());
		}

		template<typename... Args>
		ComponentView<Args...> view() {
			ComponentIterator<true, Args...> begin;
			if(!build_iterator<0>(begin)) {
				return ComponentView<Args...>();
			}
			return ComponentView<Args...>(begin, entity_count());
		}

		template<typename... Args>
		ComponentView<Args...> view(StaticArchetype<Args...>) {
			return view<Args...>();
		}

	public:
		// This can not be private because of make_unique
		Archetype(ArchetypeRuntimeInfo info = ArchetypeRuntimeInfo(), memory::PolymorphicAllocatorBase* allocator = default_allocator());

		y_serde3(_info, serde3::property(this, &Archetype::entity_count, &Archetype::set_entity_count), create_serializer_list())

	private:
		friend class EntityWorld;
		friend class ComponentSerializerList;

		void add_entities(core::MutableSpan<EntityData> entities, bool update_data);

		void transfer_to(Archetype* other, core::MutableSpan<EntityData> entities);
		void remove_entity(EntityData& data);

		void add_chunk_if_needed();
		void add_chunk();

		void pop_chunk();

		template<usize I, typename... Args>
		[[nodiscard]] bool build_iterator(ComponentIterator<true, Args...>& it) {
			static_assert(sizeof...(Args));
			if constexpr(I < sizeof...(Args)) {
				using raw_type = std::tuple_element_t<I, std::tuple<Args...>>;
				using type = remove_cvref_t<raw_type>;
				const ComponentRuntimeInfo* type_info =  _info.info_or_null<type>();
				if(!type_info) {
					return false;
				}
				it._offsets[I] = type_info->chunk_offset;
				return build_iterator<I + 1>(it);
			} else {
				it._chunks = _chunk_data.begin();
			}
			return true;
		}

		// Serialization stuff
		void set_entity_count(usize count);

		core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> create_serializers() const;
		void set_serializers(core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> serializers);

		core::Vector<ComponentSerializerWrapper> create_component_serializer_wrappers() const;
		ComponentSerializerList create_serializer_list() const;

		ArchetypeRuntimeInfo _info;

		core::Vector<void*> _chunk_data;
		usize _last_chunk_size = 0;

		void* _chunk_cache = nullptr;

		memory::PolymorphicAllocatorContainer _allocator;
};

template<typename T>
SingleComponentViewRange<T> ComponentSerializer<T>::components() const {
	return _archetype->components<T>();
}

}
}

#endif // Y_ECS_ARCHETYPE_H

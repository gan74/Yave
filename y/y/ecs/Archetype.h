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
#include "ComponentSerializer.h"

#include <y/core/Range.h>
#include <y/core/Vector.h>
#include <y/mem/allocators.h>

#include <y/serde3/serde.h>

namespace y {
namespace ecs {

class Archetype : NonMovable {

	public:
		template<typename... Args>
		static std::unique_ptr<Archetype> create() {
			auto arc = std::make_unique<Archetype>(sizeof...(Args));
			arc->set_types<0, Args...>();
			return arc;
		}

		~Archetype();

		usize entity_count() const;
		usize component_count() const;

		core::Span<ComponentRuntimeInfo> component_infos() const;

		void add_entity(EntityData& data);
		void add_entities(core::MutableSpan<EntityData> entities);


		template<typename... Args>
		ComponentView<Args...> view() {
			ComponentIterator<Args...> begin;
			if(!build_iterator<0>(begin)) {
				return ComponentView<Args...>();
			}
			return ComponentView<Args...>(begin, entity_count());
		}

	public:
		// This can not be private because of make_unique
		Archetype(usize component_count = 0, memory::PolymorphicAllocatorBase* allocator = memory::global_allocator());

		/*y_serde3(serde3::property(this, &Archetype::create_serializers, &Archetype::set_serializers),
				 serde3::property(this, &Archetype::entity_count,		&Archetype::set_entity_count),
				 create_component_serializer())*/

	private:
		friend class EntityWorld;

		void add_entities(core::MutableSpan<EntityData> entities, bool update_data);

		void sort_component_infos();
		void transfer_to(Archetype* other, core::MutableSpan<EntityData> entities);
		bool matches_type_indexes(core::Span<u32> type_indexes) const;
		void add_chunk_if_needed();
		void add_chunk();
		void remove_entity(EntityData& data);



		template<usize I, typename... Args>
		static void add_infos(ComponentRuntimeInfo* infos) {
			if constexpr(I < sizeof...(Args)) {
				using component_type = std::tuple_element_t<I, std::tuple<Args...>>;
				*infos = ComponentRuntimeInfo::from_type<component_type>();
				add_infos<I + 1, Args...>(infos + 1);
			}
		}

		template<usize I, typename... Args>
		void set_types() {
			add_infos<0, Args...>(_component_infos.get());
			sort_component_infos();
		}

		template<typename T>
		const ComponentRuntimeInfo* info_or_null() const {
			const u32 index = type_index<T>();
			Y_TODO(binary search?)
			for(usize i = 0; i != _component_count; ++i) {
				if(_component_infos[i].type_id == index) {
					return &_component_infos[i];
				}
			}
			return nullptr;
		}

		template<typename T>
		const ComponentRuntimeInfo* info() {
			if(const ComponentRuntimeInfo* i = info_or_null<T>()) {
				return i;
			}
			y_fatal("Unknown component type.");
		}

		template<usize I, typename... Args>
		[[nodiscard]] bool build_iterator(ComponentIterator<Args...>& it) {
			static_assert(sizeof...(Args));
			if constexpr(I < sizeof...(Args)) {
				using reference = typename ComponentIterator<Args...>::reference;
				using type = remove_cvref_t<std::tuple_element_t<I, reference>>;
				const ComponentRuntimeInfo* type_info =  info_or_null<type>();
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

		template<typename... Args>
		std::unique_ptr<Archetype> archetype_with() {
			static_assert(sizeof...(Args));
			auto arc = std::make_unique<Archetype>(_component_count + sizeof...(Args));
			std::copy_n(_component_infos.get(), _component_count, arc->_component_infos.get());
			add_infos<0, Args...>(arc->_component_infos.get() + _component_count);
			arc->sort_component_infos();
			return arc;
		}


		core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> create_serializers() const {
			auto serializers =  core::vector_with_capacity<std::unique_ptr<ComponentInfoSerializerBase>>(_component_count);
			for(usize i = 0; i != _component_count; ++i) {
				serializers.emplace_back(_component_infos[i].create_info_serializer());
			}
			return serializers;
		}

		 void set_serializers(core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> serializers) {
			 _component_count = serializers.size();
			 _component_infos = std::make_unique<ComponentRuntimeInfo[]>(_component_count);

			 for(usize i = 0; i != _component_count; ++i) {
				 _component_infos[i] = serializers[i]->create_runtime_info();
			 }
			 sort_component_infos();
		 }


		void set_entity_count(usize count) {
			core::MutableSpan<EntityData> entities(nullptr, count);
			add_entities(entities, false);
			y_debug_assert(entity_count() == count);
		}


		ComponentListSerializer create_component_serializer() const {
			ComponentListSerializer serializer;
			for(usize i = 0; i != _component_count; ++i) {
				serializer.add(_component_infos[i].create_component_serializer(const_cast<Archetype*>(this)));
			}
			return serializer;
		}

		usize _component_count = 0;
		std::unique_ptr<ComponentRuntimeInfo[]> _component_infos;

		core::Vector<void*> _chunk_data;
		usize _last_chunk_size = 0;

		memory::PolymorphicAllocatorContainer _allocator;
		usize _chunk_byte_size = 0;
};


}
}

#endif // Y_ECS_ARCHETYPE_H

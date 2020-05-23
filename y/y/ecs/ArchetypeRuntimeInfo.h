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
#ifndef Y_ECS_ARCHETYPERUNTIMEINFO_H
#define Y_ECS_ARCHETYPERUNTIMEINFO_H

#include "ComponentRuntimeInfo.h"
#include "ComponentSerializer.h"

#include <y/core/Range.h>
#include <y/core/Vector.h>
#include <y/mem/allocators.h>


namespace y {
namespace ecs {

class ArchetypeRuntimeInfo {
	public:
		ArchetypeRuntimeInfo() = default;

		template<typename... Args>
		static ArchetypeRuntimeInfo create() {
			ArchetypeRuntimeInfo info(sizeof...(Args));
			info.set_types<0, Args...>();
			return info;
		}

		template<typename... Args>
		ArchetypeRuntimeInfo with() {
			static_assert(sizeof...(Args));
			ArchetypeRuntimeInfo info(_component_count + sizeof...(Args));
			std::copy_n(_component_infos.get(), _component_count, info._component_infos.get());
			add_infos<0, Args...>(info._component_infos.get() + _component_count);
			info.sort_component_infos();
			return info;
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


		usize component_count() const;
		usize chunk_byte_size() const;

		core::Span<ComponentRuntimeInfo> component_infos() const;

		bool operator==(const ArchetypeRuntimeInfo& other) const;
		bool operator!=(const ArchetypeRuntimeInfo& other) const;


		y_serde3(serde3::property(this, &ArchetypeRuntimeInfo::create_serializers, &ArchetypeRuntimeInfo::set_serializers));

	private:
		ArchetypeRuntimeInfo(usize component_count);

	private:
		friend class EntityWorld;

		void sort_component_infos();

		bool matches_type_indexes(core::Span<u32> type_indexes) const;

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


		// Serialization stuff
		core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> create_serializers() const;
		void set_serializers(core::Vector<std::unique_ptr<ComponentInfoSerializerBase>> serializers);

		std::unique_ptr<ComponentRuntimeInfo[]> _component_infos;
		usize _component_count = 0;
		usize _chunk_byte_size = 0;
};

}
}

#endif // Y_ECS_ARCHETYPERUNTIMEINFO_H

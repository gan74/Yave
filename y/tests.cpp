/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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


#include <y/core/FixedArray.h>
#include <y/core/Vector.h>

#include <y/mem/allocators.h>
#include <y/core/Range.h>
#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/perf.h>
#include <y/utils/name.h>
#include <y/utils/sort.h>

#include <atomic>
#include <thread>

namespace y {
namespace detail {
static std::atomic<u32> global_type_index = 0;
}

template<typename T>
static u32 type_index() {
	static u32 index = detail::global_type_index++;
	return index;
}




class Archetype;
class EntityWorld;
class EntityBuilder;

static constexpr usize entities_per_chunk = 1024;


class EntityID {
	public:
		EntityID(u32 index = invalid_index, u32 version = 0) : _index(index), _version(version) {
		}

		u32 index() const {
			return _index;
		}

		u32 version() const {
			return _version;
		}

		bool is_valid() const {
			return _index != invalid_index;
		}

		u64 as_u64() const {
			return (u64(_index) << 32) | _version;
		}

	private:
		static constexpr u32 invalid_index = u32(-1);

		u32 _index = invalid_index;
		u32 _version = 0;
};




struct EntityData {
	EntityID id;
	Archetype* archetype = nullptr;
	usize archetype_index = 0;
};




template<typename... Args>
struct ComponentIterator {
	public:
		static constexpr usize component_count = sizeof...(Args);

		using difference_type = usize;
		using iterator_category = std::random_access_iterator_tag;

		using reference = std::tuple<Args&...>;

		reference operator*() const {
			return make_refence_tuple<0>();
		}



		bool operator==(const ComponentIterator& other) const {
			return _index == other._index && _chunks == other._chunks;
		}

		bool operator!=(const ComponentIterator& other) const {
			return !operator==(other);
		}

		ComponentIterator& operator++() {
			++_index;
			return *this;
		}

		ComponentIterator& operator--() {
			--_index;
			return *this;
		}

		ComponentIterator operator++(int) {
			const auto it = *this;
			++*this;
			return it;
		}

		ComponentIterator operator--(int) {
			const auto it = *this;
			--*this;
			return it;
		}


		difference_type operator-(const ComponentIterator& other) const {
			return _index - other._index;
		}

		ComponentIterator& operator+=(usize n) {
			_index += n;
			return *this;
		}

		ComponentIterator& operator-=(usize n) {
			_index -= n;
			return *this;
		}


		ComponentIterator operator+(usize n) const {
			auto it = *this;
			it += n;
			return it;
		}

		ComponentIterator operator-(usize n) const {
			auto it = *this;
			it -= n;
			return it;
		}


	private:
		friend class Archetype;

		template<usize I = 0>
		auto make_refence_tuple() const {
			const usize chunk_index = _index / entities_per_chunk;
			const usize item_index = _index % entities_per_chunk;
			using type = std::remove_reference_t<std::tuple_element_t<I, reference>>;

			void* offset_chunk = static_cast<u8*>(_chunks[chunk_index]) + _offsets[I];
			type* chunk = static_cast<type*>(offset_chunk);
			if constexpr(I + 1 == component_count) {
				return std::tie(chunk[item_index]);
			} else {
				return std::tuple_cat(std::tie(chunk[item_index]),
									  make_refence_tuple<I + 1>());
			}
		}

		usize _index = 0;
		void** _chunks = nullptr;
		std::array<usize, component_count> _offsets;
};




struct ComponentRuntimeInfo {
	usize chunk_offset = 0;
	usize component_size = 0;

	void (*create)(void* dst, usize count) = nullptr;
	void (*destroy)(void* ptr, usize count) = nullptr;
	void (*move)(void* dst, void* src, usize count) = nullptr;

	u32 type_id = u32(-1);

#ifdef Y_DEBUG
	std::string_view type_name = "unknown";
#endif


	void create_offset(void* chunk, usize index, usize count) const {
		create(static_cast<u8*>(chunk) + chunk_offset + index * component_size, count);
		log_msg(fmt("chunk = % + %[%]", chunk, chunk_offset, index));
	}

	void destroy_offset(void* chunk, usize index, usize count) const {
		destroy(static_cast<u8*>(chunk) + chunk_offset + index * component_size, count);
		log_msg(fmt("chunk = % + %[%]", chunk, chunk_offset, index));
	}

	void move_offset(void* dst, void* chunk, usize index, usize count) const {
		move(dst, static_cast<u8*>(chunk) + chunk_offset + index * component_size, count);
		log_msg(fmt("chunk = % + %[%]", chunk, chunk_offset, index));
	}

	template<typename T>
	static ComponentRuntimeInfo from_type(usize offset = 0) {
		return {
			offset,
			sizeof(T),
			[](void* dst, usize count) {
				log_msg(fmt("creating % %", count, ct_type_name<T>()));
				T* it = static_cast<T*>(dst);
				const T* end = it + count;
				for(; it != end; ++it) {
					::new(it) T();
				}
			},
			[](void* ptr, usize count) {
				log_msg(fmt("destroying % %", count, ct_type_name<T>()));
				T* it = static_cast<T*>(ptr);
				const T* end = it + count;
				for(; it != end; ++it) {
					it->~T();
				}
			},
			[](void* dst, void* src, usize count) {
				log_msg(fmt("moving % %", count, ct_type_name<T>()));
				T* it = static_cast<T*>(src);
				const T* end = it + count;
				T* out = static_cast<T*>(dst);
				while(it != end) {
					*out++ = std::move(*it++);
				}
			},
			type_index<T>(),

#ifdef Y_DEBUG
			ct_type_name<T>()
#endif
		};
	}
};

class Archetype : NonCopyable {

	public:
		template<typename... Args>
		static Archetype create() {
			Archetype arc(sizeof...(Args));
			arc.set_types<0, Args...>();
			arc.add_chunk();
			return arc;
		}

		Archetype(Archetype&&) = default;
		Archetype& operator=(Archetype&&) = default;

		~Archetype() {
			if(_component_infos) {
				for(usize i = 0; i != _component_count; ++i) {
					_component_infos[i].destroy_offset(_chunk_data.last(), 0, _last_chunk_size);
				}
				for(usize c = 0; c + 1 < _chunk_data.size(); ++c) {
					for(usize i = 0; i != _component_count; ++i) {
						_component_infos[i].destroy_offset(_chunk_data[c], 0, entities_per_chunk);
					}
				}

				for(void* c : _chunk_data) {
					_allocator.deallocate(c, _chunk_byte_size);
				}
			}
		}



		template<typename... Args>
		auto view() {
			ComponentIterator<Args...> begin;
			build_iterator<0>(begin);
			const auto end = begin + size();
			return core::Range(begin, end);
		}

		usize size() const {
			return (_chunk_data.size() - 1) * entities_per_chunk + _last_chunk_size;
		}

		core::Span<ComponentRuntimeInfo> component_infos() const {
			return core::Span<ComponentRuntimeInfo>(_component_infos.get(), _component_count);
		}

		void add_entity() {
			add_entities(1);
		}

		void add_entities(usize count) {
			if(_last_chunk_size == entities_per_chunk) {
				add_chunk();
			}

			const usize left_in_chunk = entities_per_chunk - _last_chunk_size;
			const usize first = std::min(left_in_chunk, count);

			void* chunk_data = _chunk_data.last();
			for(usize i = 0; i != _component_count; ++i) {
				_component_infos[i].create_offset(chunk_data, _last_chunk_size, first);
			}
			_last_chunk_size += first;

			if(first != count) {
				add_entities(count - first);
			}
		}

	private:
		friend class EntityWorld;

		Archetype(usize component_count) :
				_component_count(component_count),
				_component_infos(std::make_unique<ComponentRuntimeInfo[]>(component_count)),
				_allocator(memory::global_allocator()) {
		}

		template<usize I, typename... Args>
		void set_types() {
			if constexpr(I < sizeof...(Args)) {
				using component_type = std::tuple_element_t<I, std::tuple<Args...>>;
				_component_infos[I] = ComponentRuntimeInfo::from_type<component_type>();
				set_types<I + 1, Args...>();
			} else {
				log_msg(fmt("arch => %", ct_type_name<std::tuple<Args...>>()));
				const auto cmp = [](const ComponentRuntimeInfo& a, const ComponentRuntimeInfo& b) { return a.type_id < b.type_id; };
				sort(_component_infos.get(), _component_infos.get() + _component_count, cmp);

				y_debug_assert(_chunk_byte_size == 0);
				for(usize i = 0; i != _component_count; ++i) {
					_component_infos[i].chunk_offset = _chunk_byte_size;

					const usize size = _component_infos[i].component_size;
					_chunk_byte_size = memory::align_up_to(_chunk_byte_size, size);
					_chunk_byte_size += size * entities_per_chunk;
				}
			}
		}


		template<typename T>
		const ComponentRuntimeInfo* info() const {
			const u32 index = type_index<T>();
			for(usize i = 0; i != _component_count; ++i) {
				if(_component_infos[i].type_id == index) {
					return &_component_infos[i];
				}
			}
			return y_fatal("Unknown component type.");
		}

		template<usize I, typename... Args>
		void build_iterator(ComponentIterator<Args...>& it) {
			if constexpr(I < sizeof...(Args)) {
				using reference = typename ComponentIterator<Args...>::reference;
				using type = remove_cvref_t<std::tuple_element_t<I, reference>>;
				it._offsets[I] = info<type>()->chunk_offset;
				build_iterator<I + 1>(it);
			} else {
				it._chunks = _chunk_data.begin();
			}
		}

		void transfer_to(Archetype* arc, core::MutableSpan<EntityData> entities) {
			//y_fatal("unimplemented");
			const usize start = arc->size();
			arc->add_entities(entities.size());

			usize other_index = 0;
			for(usize i = 0; i != _component_count; ++i) {
				ComponentRuntimeInfo* other_info = nullptr;
				for(; other_index != arc->_component_count; ++other_index) {
					if(arc->_component_infos[other_index].type_id == _component_infos[i].type_id) {
						other_info = &arc->_component_infos[other_index];
						break;
					}
				}
				if(other_info) {
					for(usize e = 0; e != entities.size(); ++e) {
						EntityData& data = entities[e];
						const usize src_chunk_index = data.archetype_index / entities_per_chunk;
						const usize src_item_index = data.archetype_index % entities_per_chunk;

						const usize dst_index = start + e;
						const usize dst_chunk_index = dst_index / entities_per_chunk;
						const usize dst_item_index = dst_index % entities_per_chunk;
						const usize offset = other_info->chunk_offset + dst_item_index * other_info->component_size;
						void* dst = static_cast<u8*>(arc->_chunk_data[dst_chunk_index]) + offset;

						_component_infos[i].move_offset(dst, _chunk_data[src_chunk_index], src_item_index, 1);
					}
				}
			}

			for(usize e = 0; e != entities.size(); ++e) {
				EntityData& data = entities[e];
				data.archetype = arc;
				data.archetype_index = start + e;
			}

		}

		bool matches_type_indexes(core::Span<u32> type_indexes) const {
			y_debug_assert(std::is_sorted(type_indexes.begin(), type_indexes.end()));
			if(type_indexes.size() != _component_count) {
				return false;
			}
			for(usize i = 0; i != type_indexes.size(); ++i) {
				if(type_indexes[i] != _component_infos[i].type_id) {
					return false;
				}
			}
			return true;
		}

		void add_chunk() {
			y_debug_assert(_last_chunk_size == entities_per_chunk || _chunk_data.is_empty());
			_last_chunk_size = 0;
			_chunk_data.emplace_back(_allocator.allocate(_chunk_byte_size));
		}

		usize _component_count = 0;
		std::unique_ptr<ComponentRuntimeInfo[]> _component_infos;

		core::Vector<void*> _chunk_data;
		usize _last_chunk_size = 0;

		memory::PolymorphicAllocatorContainer _allocator;
		usize _chunk_byte_size = 0;
};





struct EntityBuilder : NonCopyable {
	public:
		EntityBuilder(EntityID id, EntityWorld& world) : _id(id), _world(world) {
		}

	private:
		EntityID _id;
		EntityWorld& _world;
};

class EntityWorld : NonCopyable {
	public:
		EntityID create_entity() {
			_entities.emplace_back();
			EntityData& ent = _entities.last();
			return ent.id = EntityID(_entities.size() - 1);
		}

		template<typename T>
		void add_component(EntityID id) {
			if(!exists(id)) {
				y_fatal("Entity doesn't exists.");
			}

			EntityData& data = _entities[id.index()];
			Archetype* old_arc = data.archetype;
			Archetype* new_arc = nullptr;

			core::Vector<u32> types;
			{
				{
					if(old_arc) {
						for(const ComponentRuntimeInfo& info : old_arc->component_infos()) {
							types << info.type_id;
						}
					}
					types << type_index<T>();
					sort(types.begin(), types.end());
				}

				for(Archetype& arc : _archetypes) {
					if(arc.matches_type_indexes(types)) {
						new_arc = &arc;
						break;
					}
				}	

				if(!new_arc) {
					new_arc = &_archetypes.emplace_back(Archetype::create<T>()); // THIS IS FUCKED
				}
			}

			y_debug_assert(new_arc->_component_count == types.size());
			if(old_arc) {
				old_arc->transfer_to(new_arc, data);
			} else {
				new_arc->add_entity();
				data.archetype = new_arc;
			}
		}

		bool exists(EntityID id) const {
			if(id.index() >= _entities.size()) {
				return false;
			}
			return _entities[id.index()].id.version() == id.version();
		}

	private:
		core::Vector<EntityData> _entities;
		core::Vector<Archetype> _archetypes;
};
}


using namespace y;


struct Tester : NonCopyable {
	Tester() : x(0xFDFEFDFE12345678) {
	}

	Tester(Tester&& other) : x(other.x) {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
		other.x = 0x0F0F0F0F0F0F0F0F;
		log_msg("moved");
	}

	Tester& operator=(Tester&&) {
		y_fatal("!");
		return *this;
	}

	~Tester() {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
		log_msg("destroyed");
	}

	u64 x = 0;
};

int main() {
	EntityWorld world;

	EntityID id = world.create_entity();
	world.add_component<Tester>(id);
	world.add_component<int>(id);
	world.add_component<float>(id);

	/*for(auto [i] : arc.view<const int>()) {
		log_msg(fmt("% %", ct_type_name<decltype(i)>(), i));
	}*/

	log_msg("Ok");
	return 0;
}




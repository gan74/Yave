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

#include <y/ecs/ecs.h>
#include <y/ecs/Archetype.h>

#include <y/core/FixedArray.h>
#include <y/core/Vector.h>
#include <y/core/String.h>

#include <y/mem/allocators.h>
#include <y/core/Range.h>
#include <y/math/Vec.h>
#include <y/utils/log.h>
#include <y/utils/perf.h>
#include <y/utils/name.h>
#include <y/utils/sort.h>

#include <atomic>
#include <thread>

using namespace y;
using namespace y::ecs;

/*
struct EntityBuilder : NonCopyable {
	public:
		EntityBuilder(EntityID id, EntityWorld& world) : _id(id), _world(world) {
		}

	private:
		EntityID _id;
		EntityWorld& _world;
};*/

namespace y {
namespace ecs {

class EntityWorld : NonCopyable {
	public:
		EntityID create_entity() {
			_entities.emplace_back();
			EntityData& ent = _entities.last();
			return ent.id = EntityID(_entities.size() - 1);
		}

		void remove_entity(EntityID id) {
			check_exists(id);

			EntityData& data = _entities[id.index()];
			if(!data.archetype) {
				data.invalidate();
			} else {
				data.archetype->remove_entity(data);
			}
		}


		core::Span<std::unique_ptr<Archetype>> archetypes() const {
			return _archetypes;
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

		bool exists(EntityID id) const {
			if(id.index() >= _entities.size()) {
				return false;
			}
			return _entities[id.index()].id.version() == id.version();
		}

	private:
		template<usize I, typename... Args>
		static void add_type_indexes(core::Vector<u32>& types) {
			static_assert(sizeof...(Args));
			if constexpr(I < sizeof...(Args)) {
				using type = std::tuple_element_t<I, std::tuple<Args...>>;
				types << type_index<type>();
				add_type_indexes<I + 1, Args...>(types);
			}
		}

		void transfer(EntityData& data, Archetype* to) {
			y_debug_assert(data.archetype != to);
			if(data.archetype) {
				data.archetype->transfer_to(to, data);
			} else {
				to->add_entity(data);
			}
			y_debug_assert(data.archetype == to);
		}

		void check_exists(EntityID id) const {
			if(!exists(id)) {
				y_fatal("Entity doesn't exists.");
			}
		}

		core::Vector<EntityData> _entities;
		core::Vector<std::unique_ptr<Archetype>> _archetypes;
};

}
}


struct Tester : NonCopyable {
	Tester() : x(0xFDFEFDFE12345678) {
		validate();
	}

	Tester(Tester&& other) : x(other.x) {
		other.validate();
		validate();
		other.x = 0x0F0F0F0F0F0F0F0F;
	}

	Tester& operator=(Tester&& other) {
		validate();
		other.validate();
		x = other.x;
		other.x = 0x0F0F0F0F0F0F0F0F;
		return *this;
	}

	~Tester() {
		validate();
		x = 1;
		log_msg("destroyed");
	}

	void validate() const {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
	}

	u64 x = 0;
};

int main() {
	EntityWorld world;

	{
		EntityID id = world.create_entity();
		world.add_component<int>(id);
		world.add_component<Tester>(id);
		world.add_component<float>(id);
	}
	{
		EntityID id = world.create_entity();
		world.add_components<Tester, int>(id);
	}
	{
		EntityID id = world.create_entity();
		world.add_component<int>(id);
	}

	for(const auto& a : world.archetypes()) {
		core::String comps;
		bool has_tester = false;
		for(const auto& info : a->component_infos()) {
			if(info.type_id == type_index<Tester>()) {
				has_tester = true;
			}
#ifdef Y_DEBUG
			comps = comps + info.type_name + " ";
#endif
		}
		log_msg(fmt("[%]< %> = %", a->component_count(), comps, a->size()));

		if(has_tester) {
			for(auto [t] : a->view<const Tester>()) {
				static_assert(std::is_same_v<decltype(t), const Tester&>);
				t.validate();
			}
		}
		log_msg("");
	}

	/*for(auto [i] : arc.view<const int>()) {
		log_msg(fmt("% %", ct_type_name<decltype(i)>(), i));
	}*/

	log_msg("Ok");
	return 0;
}




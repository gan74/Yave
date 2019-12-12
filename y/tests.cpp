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


		core::Span<Archetype> archetypes() const {
			return _archetypes;
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
					if(old_arc) {
						new_arc = &_archetypes.emplace_back(old_arc->archetype_with<T>());
					} else {
						new_arc = &_archetypes.emplace_back(Archetype::create<T>());
					}
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
}


struct Tester : NonCopyable {
	Tester() : x(0xFDFEFDFE12345678) {
	}

	Tester(Tester&& other) : x(other.x) {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
		other.x = 0x0F0F0F0F0F0F0F0F;
		log_msg("moved");
	}

	Tester& operator=(Tester&& other) {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
		x = other.x;
		other.x = 0x0F0F0F0F0F0F0F0F;
		return *this;
	}

	~Tester() {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
		x = 1;
		log_msg("destroyed");
	}

	u64 x = 0;
};

int main() {
	EntityWorld world;

	{
		EntityID id = world.create_entity();
		world.add_component<Tester>(id);
		world.add_component<int>(id);
		world.add_component<float>(id);
	}
	{
		EntityID id = world.create_entity();
		world.add_component<Tester>(id);
		world.add_component<int>(id);
	}
	/*{
		EntityID id = world.create_entity();
		world.add_component<int>(id);
	}*/

	for(const Archetype& a : world.archetypes()) {
		core::String comps;
		for(const auto info : a.component_infos()) {
			comps = comps + info.type_name + " ";
		}
		log_msg(fmt("[%]< %> = %", a.component_count(), comps, a.size()));
	}

	/*for(auto [i] : arc.view<const int>()) {
		log_msg(fmt("% %", ct_type_name<decltype(i)>(), i));
	}*/

	log_msg("Ok");
	return 0;
}




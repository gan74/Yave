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

#include <y/ecs/ecs.h>
#include <y/ecs/Archetype.h>
#include <y/ecs/EntityWorld.h>
#include <y/ecs/EntityView.h>

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

struct NotComp {
};

int main() {
	log_msg(ct_type_name<int>());
	log_msg(ct_type_name<float>());
	//return 0;


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

	/*for(const auto& a : world.archetypes()) {
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
	}*/


	for(auto&& [e, n] : world.view<Tester, NotComp>()) {
		e.validate();
		log_msg("!");
	}

	/*for(auto [i] : arc.view<const int>()) {
		log_msg(fmt("% %", ct_type_name<decltype(i)>(), i));
	}*/

	log_msg("Ok");
	return 0;
}




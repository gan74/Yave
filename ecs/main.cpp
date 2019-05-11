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

#include <yave/yave.h>

#include <y/core/Chrono.h>
#include <y/math/random.h>

#include "EntityWorld.h"

using namespace yave;
using namespace ecs;

template<usize N>
static usize popcnt_(std::bitset<N> t, usize bit) {
	t <<= N - bit;
	return t.count();
}

void test_bitset() {
	std::bitset<32> set;
	set[1] = true;
	set[4] = true;
	set[7] = true;
	set[12] = true;
	set[25] = true;
	y_debug_assert(popcnt_(set, 1) == 0);
	y_debug_assert(popcnt_(set, 12) == 3);
	y_debug_assert(popcnt_(set, 13) == 4);
}

void test_multi() {
	struct A {};
	struct B {};
	struct C {};
	EntityWorld world;
	auto tag = [&, i = 0](EntityId id) mutable { *world.component<usize>(world.add_component<usize>(id)) = i++; };
	{
		EntityId id = world.create_entity();
		world.add_component<A>(id);
		world.add_component<B>(id);
		world.add_component<C>(id);
		tag(id);
	}
	{
		EntityId id = world.create_entity();
		world.add_component<A>(id);
		world.add_component<B>(id);
		tag(id);
	}
	{
		EntityId id = world.create_entity();
		world.add_component<A>(id);
		world.add_component<C>(id);
		tag(id);
	}

	{
		auto multi = world.entities_with<A, B>();
		y_debug_assert(multi.size() == 2);
		unused(multi);
	}
	{
		auto multi = world.entities_with<A>();
		y_debug_assert(multi.size() == 3);
		unused(multi);
	}
	{
		auto multi = world.entities_with<C>();
		y_debug_assert(multi.size() == 2);
		unused(multi);
	}
	{
		auto multi = world.entities_with<C, A>();
		y_debug_assert(multi.size() == 2);
		unused(multi);
	}
	{
		auto multi = world.entities_with<C, A, B>();
		y_debug_assert(multi.size() == 1);
		unused(multi);
	}
}


template<typename C>
usize count_components(const EntityWorld& w) {
	auto c = w.components<C>();
	return std::distance(c.begin(), c.end());
}

void test_perf(usize max = 100000) {

#define ADD_CMP(Type) do { if(rng() % 2) { world.add_component<Type>(id); } } while(false)
#define REM_CMP(Type) do { world.remove_component<Type>(entities[i]); } while(false)

	struct A {};
	struct B {};
	struct C {};
	struct D {};
	struct E {};
	struct F {};
	struct G {};

	math::FastRandom rng;
	EntityWorld world;
	auto entities = core::vector_with_capacity<EntityId>(max);

	{
		core::DebugTimer _("Adding entities");
		for(usize i = 0; i != max; ++i) {
			EntityId id = world.create_entity();
			entities << id;
			world.add_component<A>(id);
			ADD_CMP(B);
			ADD_CMP(C);
			ADD_CMP(D);
			ADD_CMP(E);
			ADD_CMP(F);
			ADD_CMP(G);
		}
	}

	log_msg(fmt("A: %", count_components<A>(world)));
	log_msg(fmt("B: %", count_components<B>(world)));
	log_msg(fmt("C: %", count_components<C>(world)));
	log_msg(fmt("D: %", count_components<D>(world)));
	log_msg(fmt("E: %", count_components<E>(world)));
	log_msg(fmt("F: %", count_components<F>(world)));
	log_msg(fmt("G: %", count_components<G>(world)));


	{
		core::DebugTimer _("Removing all");
		for(usize i = 0; i != max; ++i) {
			REM_CMP(C);
		}
	}
	{
		core::DebugTimer _("Removing 1/2");
		std::shuffle(entities.begin(), entities.end(), math::FastRandom(rng()));
		for(usize i = 0; i != max / 2; ++i) {
			REM_CMP(D);
		}
	}
	{
		core::DebugTimer _("Removing 1/4");
		std::shuffle(entities.begin(), entities.end(), math::FastRandom(rng()));
		for(usize i = 0; i != max / 4; ++i) {
			REM_CMP(E);
		}
	}
	{
		core::DebugTimer _("Removing 1/8");
		std::shuffle(entities.begin(), entities.end(), math::FastRandom(rng()));
		for(usize i = 0; i != max / 8; ++i) {
			REM_CMP(F);
		}
	}
	{
		core::DebugTimer _("Removing 1/16");
		std::shuffle(entities.begin(), entities.end(), math::FastRandom(rng()));
		for(usize i = 0; i != max / 16; ++i) {
			REM_CMP(G);
		}
	}

	{
		core::DebugTimer _("Flush");
		world.flush();
	}

#undef ADD_CMP
#undef REM_CMP


	log_msg(fmt("A: %", count_components<A>(world)));
	log_msg(fmt("B: %", count_components<B>(world)));
	log_msg(fmt("C: %", count_components<C>(world)));
	log_msg(fmt("D: %", count_components<D>(world)));
	log_msg(fmt("E: %", count_components<E>(world)));
	log_msg(fmt("F: %", count_components<F>(world)));
	log_msg(fmt("G: %", count_components<G>(world)));

	usize abd_count = 0;
	{
		core::DebugTimer _("Multi 3");
		const EntityWorld& cw = world;

		auto multi = cw.ids_with<D, A, B>();
		bool all_ok = true;
		for(EntityId id : multi) {
			all_ok &= !!world.component<A>(id);
			all_ok &= !!world.component<B>(id);
			all_ok &= !!world.component<D>(id);
			y_debug_assert(all_ok);
			++abd_count;
		}
		if(!all_ok) {
			y_fatal("Missing components.");
		}
		log_msg(fmt("A, B, D: %", abd_count));
	}
	{
		core::DebugTimer _("Checking count");
		usize count = 0;
		for(const Entity& e : world.entities()) {
			if(world.component<A>(e.id()) && world.component<B>(e.id()) &&world.component<D>(e.id())) {
				++count;
			}
		}
		if(count != abd_count) {
			y_fatal("Multi failure.");
		}
	}

}

void test_remove() {

}

int main(int, char**) {
	log_msg("Hello entities");

	test_multi();
	test_remove();
	test_perf();

	log_msg("Ok!");

	return 0;
}

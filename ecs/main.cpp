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

/*#include <yave/yave.h>

#include <y/core/Chrono.h>

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

int main(int, char**) {
	log_msg("Hello world");

	EntityWorld world;

	y_debug_assert([] { log_msg("asserts enabled"); return true; }());
	y_debug_assert(world.entity(EntityId()) == nullptr);

	EntityId id = world.create_entity();
	y_debug_assert(world.entity(id) != nullptr);
	y_debug_assert(world.entity(EntityId()) == nullptr);
	const Entity* ent = world.entity(id);
	world.remove_entity(id);
	y_debug_assert(world.entity(id) != nullptr);
	world.flush();
	y_debug_assert(world.entity(id) == nullptr);
	EntityId id2 = world.create_entity();
	y_debug_assert(id != id2);
	y_debug_assert(world.entity(id) == nullptr);
	y_debug_assert(world.entity(id2) != nullptr);

	ComponentId cmp = world.add_component<int>(id2);
	y_debug_assert(world.components<int>().size() == 1);
	y_debug_assert(world.components<double>().size() == 0);
	*world.component<int>(cmp) = 7;
	y_debug_assert(world.components<int>().size() == 1);
	y_debug_assert(*world.component<int>(cmp) == 7);

	for(int i : world.components<int>()) {
		unused(i);
		y_debug_assert(i == 7);
	}

	test_bitset();

	unused(ent, cmp);


	log_msg("Ok!");

	return 0;
}*/



#include <y/io2/io.h>
#include <y/io2/File.h>
#include <y/io2/BuffReader.h>
#include <y/io2/Buffer.h>
#include <y/serde2/archives.h>

using namespace y;


struct S {
	int x;
	core::String str;

	y_serde2(x, str)
};

struct Test {
	u32 a;
	core::String str;
	std::tuple<i32, char> tpl;
	core::Vector<S> vec;


	y_serde2(a, str, tpl, vec)
};

int main(int, char**) {
	io2::Buffer buffer;

	const char* str = "flubudu lalalalalalalalalalalala lol";
	log_msg(fmt("%", core::ArrayView<char>(str, std::strlen(str))));
	{
		Test t{7, str, {-12, 'l'}, {S{8, "huit"}, S{17, "dix sept"}}};
		{
			io2::Writer writer(buffer);
			serde2::WritableArchive<> ar(writer);
			ar(t);
		}

	}

	{
		io2::Reader reader(buffer);
		serde2::ReadableArchive<> ar(reader);
		Test t;
		ar(t);
		log_msg(fmt("Test{%, %, %}", t.a, t.str, t.tpl));
		for(auto&& s : t.vec) {
			log_msg(fmt("Vec{%, %}", s.x, s.str));
		}
	}

	log_msg("ok");

	return 0;
}

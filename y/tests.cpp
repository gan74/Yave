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
#include <y/utils/iter.h>

#include <y/serde3/poly.h>
#include <y/serde3/serde.h>
#include <y/serde3/archives.h>
#include <y/io2/Buffer.h>

#include <atomic>
#include <thread>

using namespace y;
using namespace y::ecs;

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

struct Test {
	int i = 4;

	const int& ser() const {
		return i;
	}

	int& ser() {
		return i;
	}

	y_serde3(ser())
};





struct ComponentSerializerBase : NonMovable {
	virtual ~ComponentSerializerBase() {
	}

	y_serde3_poly_base(ComponentSerializerBase)
};

template<typename T>
class ComponentSerializerIterator {
	public:

	private:
};

template<typename T>
struct ComponentSerializer final : ComponentSerializerBase {




	y_serde3_poly(ComponentSerializer)
};




template<typename T>
struct ComponentTransform {
	ComponentTransform(const EntityWorld& world) : _world(world) {
	}

	std::pair<EntityID, T*> operator()(const EntityData& data) const {
		y_debug_assert(data.id.is_valid());
		return {data.id, _world.component<T>(data)};
	}

	const EntityWorld& _world;
};


struct NonNullComponentFilter {
	template<typename T>
	bool operator()(const std::pair<EntityID, T*>& entity) const {
		return entity.second;
	}
};



template<typename T>
auto all_components(const EntityWorld& world) {
	auto entities = world.entities();
	auto component_beg = TransformIterator(entities.begin(), ComponentTransform<T>(world));
	auto non_null_beg = FilterIterator(component_beg, entities.end(), NonNullComponentFilter());
	auto deref = [](const std::pair<EntityID, T*>& p) -> std::pair<EntityID, T&> { return {p.first, *p.second}; };
	return core::Range(TransformIterator(non_null_beg, deref), entities.end());
}


int main() {
	auto file = io2::Buffer();

	EntityWorld world;

	{
		EntityID id = world.create_entity();
		log_msg(fmt("a: %", id.index()));
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

	log_msg("Testers:");
	for(auto&& [id, tester] : all_components<Tester>(world)) {
		y_debug_assert(id.is_valid());
		log_msg(fmt("id: [%, %]", id.index(), id.version()));
		tester.validate();
	}

	log_msg("int:");
	for(auto&& [id, i] : all_components<int>(world)) {
		y_debug_assert(id.is_valid());
		log_msg(fmt("id: [%, %]", id.index(), id.version()));
		log_msg(fmt("int: %", i));
	}



	log_msg("Ok");
	return 0;
}




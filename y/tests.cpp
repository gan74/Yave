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

#include <y/concurrent/StaticThreadPool.h>

#include <y/core/FixedArray.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/core/Chrono.h>

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
#include <y/serde3/property.h>
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
	}

	void validate() const {
		y_debug_assert(x == 0x0F0F0F0F0F0F0F0F || x == 0xFDFEFDFE12345678);
	}

	u64 x = 0;



	y_serde3(serde_data())

	const u64& serde_data() const {
		//log_msg("serialization");
		return x;
	}

	u64& serde_data() {
		//log_msg("deserialization");
		return x;
	}
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


template<typename T>
[[maybe_unused]] static auto all_components(EntityWorld& world) {
	auto extract_component = [&world](EntityID id) -> std::pair<EntityID, T*> {
		y_debug_assert(id.is_valid());
		return {id, world.component<T>(id)};
	};
	auto non_null = [](const std::pair<EntityID, T*>& p) -> bool {
		//log_msg(fmt("testing entity %, for component % (result = %)", p.first.index(), ct_type_name<T>(), p.second));
		return p.second;
	};
	auto deref = [](const std::pair<EntityID, T*>& p) -> std::pair<EntityID, T&> {
		return {p.first, *p.second};
	};

	auto ids = world.entity_ids();
	auto components = TransformIterator(ids.begin(), extract_component);
	auto non_nulls = FilterIterator(components, ids.end(), non_null);
	return core::Range(TransformIterator(non_nulls, deref), EndIterator());
}



struct W {
	int w;

	int test() const {
		return 4;
	}

	y_serde3(test())
};



struct M {
	int i = 0;

	void set_i(int x) {
		log_msg(fmt("set_i << %", x));
		i = x;
	}

	int get_i() const {
		log_msg(fmt("get_i >> %", i));
		return i;
	}

	y_serde3(serde3::property(this, &M::get_i, &M::set_i))

	static decltype(auto) prop(M* s) {
		return serde3::property(s, &M::get_i, &M::set_i);
	}

};


static_assert(serde3::is_property_v<decltype(M::prop(nullptr))>);

int main() {
#ifdef Y_DEBUG
	core::result::break_on_error = true;
#endif

	y_debug_assert([]() { log_msg("Debug asserts enabled", Log::Debug); return true; }());

#if 0
	std::array ms = {M{1}, M{2}};
	core::MutableSpan<M> m_span = ms;

	auto file = io2::Buffer();
	{
		serde3::WritableArchive arc(file);
		arc.serialize(m_span).unwrap();
		log_msg("Serialization ok");
	}
	file.reset();
	{
		serde3::ReadableArchive arc(file);
		arc.deserialize(m_span).unwrap();
		log_msg("Deserialization ok");
	}
#endif

#if 1
	EntityWorld world;

	{
		EntityID id = world.create_entity();
		world.add_component<int>(id);
		world.add_component<Tester>(id);
		world.add_component<float>(id);
	}
	world.add_on_create<Tester>([](const EntityWorld&, EntityID id) { log_msg(fmt("Tester added to entity #%", id.index())); });
	{
		EntityID id = world.create_entity();
		world.add_components<Tester, int>(id);
		world.add_components<Tester, int>(world.create_entity());
		world.add_components<Tester, int>(world.create_entity());
		world.add_components<Tester, int>(world.create_entity());
	}
	{
		EntityID id = world.create_entity();
		world.add_component<int>(id);
	}

	W w;

	try {
		EntityWorld new_world;
		new_world.add_on_create<Tester>([](const EntityWorld&, EntityID id) { log_msg(fmt("Tester added to new world's entity #%", id.index())); });
		auto file = io2::Buffer();
		{
			log_msg("Serialization...");
			serde3::WritableArchive arc(file);
			arc.serialize(w).or_throw();
			arc.serialize(world).or_throw();
			log_msg("Serialization ok");
		}
		file.reset();
		{
			log_msg("Deserialization...");
			serde3::ReadableArchive arc(file);
			arc.deserialize(w).or_throw();
			arc.deserialize(new_world).or_throw();
			log_msg("Deserialization ok");
		}


		auto print_world = [](EntityWorld& world) {
			usize ents = 0;
			for(auto&& id : world.entity_ids()) {
				unused(id);
				++ents;
			}
			log_msg(fmt("World entity count: %", ents));
			log_msg("  Archetypes:");
			for(const auto& arc : world.archetypes()) {
				log_msg(fmt("    entity count: %", arc->entity_count()));
				log_msg(fmt("    component count: %", arc->component_count()));
				for(const ComponentRuntimeInfo& info : arc->component_infos()) {
					log_msg(fmt("      %", info.type_name));
				}
			}
			log_msg("  Testers:");
			for(auto&& [id, tester] : all_components<Tester>(world)) {
				y_debug_assert(id.is_valid());
				log_msg(fmt("    id: [%, %]", id.index(), id.version()));
				tester.validate();
			}

			log_msg("  int:");
			for(auto&& [id, i] : all_components<int>(world)) {
				y_debug_assert(id.is_valid());
				y_debug_assert(i == 0);
				log_msg(fmt("    id: [%, %]", id.index(), id.version()));
			}
		};

		log_msg("Original world:");
		print_world(world);

		log_msg("Deser world:");
		print_world(new_world);

	} catch(serde3::Error err) {
		log_msg(fmt("Serde error: % while processing: \"%\"", serde3::error_msg(err), err.member ? err.member : ""), Log::Error);
	}

	log_msg("-------------------------");

#endif
	return 0;
}




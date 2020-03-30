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


/*template<typename T>
struct ComponentTransform {
	ComponentTransform(EntityWorld& world) : _world(world) {
	}

	std::pair<EntityID, T*> operator()(EntityID id) const {
		y_debug_assert(id.is_valid());
		return {id, _world.component<T>(id)};
	}

	EntityWorld& _world;
};


struct NonNullComponentFilter {
	template<typename T>
	bool operator()(const std::pair<EntityID, T*>& entity) const {
		return entity.second;
	}
};



template<typename T>
auto all_components(EntityWorld& world) {
	auto ids = world.entity_ids();
	auto component_beg = TransformIterator(ids.begin(), ComponentTransform<T>(world));
	auto non_null_beg = FilterIterator(component_beg, ids.end(), NonNullComponentFilter());
	auto deref = [](const std::pair<EntityID, T*>& p) -> std::pair<EntityID, T&> { return {p.first, *p.second}; };
	return core::Range(TransformIterator(non_null_beg, deref), EndIterator());
}*/



namespace y {
namespace ecs {

class ComponentSerializerBase : NonMovable {
	public:
		virtual ~ComponentSerializerBase() {
		}

		y_serde3_poly_base(ComponentSerializerBase)

	protected:
		template<typename T>
		static auto id_components(const EntityWorld& world) {
			auto extract_component = [&world](const EntityData& data) -> std::pair<EntityID, T*> {
				y_debug_assert(data.id.is_valid());
				return {data.id, world.find_component<T>(data)};
			};
			auto non_null = [](const std::pair<EntityID, T*>& p) -> bool {
				return p.second;
			};
			auto deref = [](const std::pair<EntityID, T*>& p) -> std::pair<EntityID, T&> {
				return {p.first, *p.second};
			};

			const auto& entities = world._entities;
			auto components = TransformIterator(entities.begin(), extract_component);
			auto non_nulls = FilterIterator(components, entities.end(), non_null);
			return core::Range(TransformIterator(non_nulls, deref), EndIterator());
		}
};

template<typename T>
class ComponentSerializer final : public ComponentSerializerBase {
	int dummy = 0;

	y_serde3_poly(ComponentSerializer)
	y_serde3(dummy)
};

}
}


template<typename T>
static auto all_components(EntityWorld& world) {
	auto extract_component = [&world](EntityID id) -> std::pair<EntityID, T*> {
		y_debug_assert(id.is_valid());
		return {id, world.component<T>(id)};
	};
	auto non_null = [](const std::pair<EntityID, T*>& p) -> bool {
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

int main() {
	auto file = io2::Buffer();

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
		world.add_components<Tester, int>(world.create_entity());
		world.add_components<Tester, int>(world.create_entity());
		world.add_components<Tester, int>(world.create_entity());
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
		y_debug_assert(i == 0);
		log_msg(fmt("id: [%, %]", id.index(), id.version()));
	}



	log_msg("Ok");
	return 0;
}




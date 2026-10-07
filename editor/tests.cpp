/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

// Test executable for the ECS (EntityWorld and friends).
// Built as the "editor_tests" target, not part of the editor itself.

#include <yave/ecs/EntityWorld.h>
#include <yave/systems/TriggerSystem.h>
#include <yave/systems/TimeSystem.h>
#include <yave/systems/JoltPhysicsSystem.h>
#include <yave/components/BlueprintComponent.h>
#include <yave/blueprints/TriggerBlueprintNode.h>
#include <yave/blueprints/blueprint_nodes.h>
#include <yave/components/TransformableComponent.h>
#include <yave/scene/SpatialPartition.h>
#include <yave/utils/IndexAllocator.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/assets/FolderAssetStore.h>
#include <yave/meshes/AABB.h>

#include <y/concurrent/JobSystem.h>
#include <y/serde3/archives.h>
#include <y/io2/Buffer.h>

#include <y/core/HashMap.h>
#include <y/math/random.h>
#include <y/test/test.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <optional>
#include <random>
#include <thread>
#include <unordered_map>



// Components live in a named namespace so that their clean names are predictable
namespace ecstest {

using namespace yave;
using namespace yave::ecs;

struct Position {
    float x = 0.0f;
    float y = 0.0f;

    Position() = default;
    Position(float a, float b) : x(a), y(b) {
    }

    y_reflect(Position, x, y)
};

struct Velocity {
    float dx = 0.0f;

    Velocity() = default;
    Velocity(float d) : dx(d) {
    }

    y_reflect(Velocity, dx)
};

struct Dependent : RequireComponent<Position> {
    u32 value = 0;

    Dependent() = default;
    Dependent(u32 v) : value(v) {
    }

    y_reflect(Dependent, value)
};

struct DeepDependent : RequireComponent<Dependent> {
    u32 value = 0;

    y_reflect(DeepDependent, value)
};

struct Link {
    EntityId target;

    Link() = default;
    Link(EntityId t) : target(t) {
    }

    y_reflect(Link, target)
};

struct Named {
    core::String name;
    u32 count = 0;

    Named() = default;
    Named(core::String n, u32 c = 0) : name(std::move(n)), count(c) {
    }

    void inspect(ComponentInspector* inspector) {
        inspector->inspect("name", name);
        inspector->inspect("count", count);
    }

    y_reflect(Named, name, count)
};

class RegisteringSystem;

struct Registered : RegisterComponent<Registered, RegisteringSystem> {
    u32 value = 0;

    y_reflect(Registered, value)
};


class RegisteringSystem : public System {
    public:
        RegisteringSystem() : System("RegisteringSystem") {
        }

        void setup(SystemScheduler&) override {
        }

        template<typename T>
        void register_component_type() {
            registered << type_index<T>();
        }

        core::Vector<ComponentTypeIndex> registered;
};


struct TestTrigger {
    u32 value = 0;
};

struct OtherTestTrigger {
    u32 value = 0;
};

struct TriggerListening : RegisterComponent<TriggerListening, TriggerSystem> {
    bool listen_test = true;
    bool listen_other = false;

    mutable core::Vector<u32> received;

    void subscribe_triggers(TriggerSubscriber& subscriber) const {
        if(listen_test) {
            subscriber.subscribe<TestTrigger>();
            subscriber.subscribe<TestTrigger>();
        }
        if(listen_other) {
            subscriber.subscribe<OtherTestTrigger>();
        }
    }

    void on_trigger(EntityWorld&, EntityId, TriggerTypeIndex type, const void* payload) const {
        if(type == trigger_index<TestTrigger>()) {
            received << static_cast<const TestTrigger*>(payload)->value;
        } else {
            received << static_cast<const OtherTestTrigger*>(payload)->value + 100;
        }
    }

    y_reflect(TriggerListening, listen_test, listen_other)
};

struct BlueprintTestTrigger {
    float value = 0.0f;

    y_reflect(BlueprintTestTrigger, value)
};

struct OtherBlueprintTestTrigger {
    float value = 0.0f;

    y_reflect(OtherBlueprintTestTrigger, value)
};

// Writes its input to outputs[index] when executed
class TestOutputBlueprintNode final : public BlueprintNode {
    static inline const BlueprintPin static_input_pin = { "in", blueprint_param_type<float>() };

    public:
        static inline std::array<float, 4> outputs = {};

        TestOutputBlueprintNode(u32 index = 0) : BlueprintNode("output"), _index(index) {
        }

        std::string_view node_type_name() const override {
            return "TestOutput";
        }

        core::Span<BlueprintPin> input_pins() const override {
            return static_input_pin;
        }

        void* default_input(usize) override {
            return &_default;
        }

        void compile(BlueprintCompiler& compiler) const override {
            const float* in = static_cast<const float*>(compiler.input(0));
            compiler.emit([in, index = _index] { outputs[index] = *in; });
        }

        y_reflect(TestOutputBlueprintNode, _name, _index)
        y_serde3_poly(TestOutputBlueprintNode)

    private:
        u32 _index = 0;
        float _default = 0.0f;
};


class RecordingInspector final : public TemplateComponentInspector<RecordingInspector> {
    public:
        bool inspect_component_type(ComponentRuntimeInfo info, bool has_inspect) override {
            types << info.type_id;
            has_inspects << has_inspect;
            return accept;
        }

        template<typename T>
        void visit(const core::String& name, T& t) {
            visited << name;
            if constexpr(std::is_same_v<T, core::String>) {
                if(!new_name.is_empty()) {
                    t = new_name;
                }
            }
        }

        bool accept = true;
        core::String new_name;

        core::Vector<ComponentTypeIndex> types;
        core::Vector<bool> has_inspects;
        core::Vector<core::String> visited;
};

}


namespace {

using namespace y;
using namespace yave;
using namespace yave::ecs;
using namespace ecstest;

template<typename T>
const ComponentContainerBase* container_of(const EntityWorld& world) {
    for(const ComponentContainerBase* container : world.component_containers()) {
        if(container->type_id() == type_index<T>()) {
            return container;
        }
    }
    return nullptr;
}

template<typename R>
core::Vector<EntityId> collect(R&& range) {
    core::Vector<EntityId> ids;
    for(const EntityId id : range) {
        ids << id;
    }
    return ids;
}

template<typename C>
bool contains(const C& c, EntityId id) {
    return std::find(c.begin(), c.end(), id) != c.end();
}

template<typename C>
bool contains_type(const C& c, ComponentTypeIndex type) {
    return std::find(c.begin(), c.end(), type) != c.end();
}

bool same_ids(core::Span<EntityId> a, core::Span<EntityId> b) {
    if(a.size() != b.size()) {
        return false;
    }
    for(const EntityId id : a) {
        if(std::find(b.begin(), b.end(), id) == b.end()) {
            return false;
        }
    }
    return true;
}

void save_and_load(const EntityWorld& from, EntityWorld& to) {
    io2::Buffer buffer;
    {
        serde3::WritableArchive arc(buffer);
        from.save_state(arc).expected("Unable to save world");
    }
    buffer.reset();
    {
        serde3::ReadableArchive arc(buffer);
        to.load_state(arc).expected("Unable to load world");
    }
}




// ---------------------------------------- Basic types ----------------------------------------

y_test_func("EntityId basics") {
    const EntityId invalid;
    y_test_assert(!invalid.is_valid());

    const EntityId id(3, 7);
    y_test_assert(id.is_valid());
    y_test_assert(id.index() == 3);
    y_test_assert(id.version() == 7);
    y_test_assert(EntityId::from_u64(id.as_u64()) == id);
    y_test_assert(id.as_u64() == ((u64(3) << 32) | 7));

    y_test_assert(EntityId(3, 7) == id);
    y_test_assert(EntityId(3, 8) != id);
    y_test_assert(EntityId(4, 7) != id);
    y_test_assert(EntityId(2, 9) < id);
    y_test_assert(EntityId(3, 6) < id);

    y_test_assert(std::hash<EntityId>()(id) == std::hash<EntityId>()(EntityId(3, 7)));

    const EntityId dummy = EntityId::dummy(5);
    y_test_assert(dummy.is_valid());
    y_test_assert(dummy.index() == 5);
    y_test_assert(dummy.version() == u32(-1));

    EntityId mut = id;
    mut.invalidate();
    y_test_assert(!mut.is_valid());
    y_test_assert(mut.version() == 7);
    mut.make_valid(12);
    y_test_assert(mut.is_valid());
    y_test_assert(mut.index() == 12);
    y_test_assert(mut.version() == 8);
}

y_test_func("TickId ordering") {
    const TickId a;
    const TickId b = a.next();
    const TickId c = b.next();

    y_test_assert(a == TickId());
    y_test_assert(a != b);
    y_test_assert(a < b);
    y_test_assert(b < c);
    y_test_assert(c > a);
    y_test_assert(a.next() == b);
}

y_test_func("SparseIdSet insert/erase") {
    SparseIdSet set;
    y_test_assert(set.is_empty());
    y_test_assert(set.size() == 0);
    y_test_assert(!set.contains(EntityId(0, 1)));

    for(u32 i = 0; i != 64; ++i) {
        y_test_assert(set.insert(EntityId(i * 3, 1)));
    }
    y_test_assert(set.size() == 64);
    y_test_assert(!set.insert(EntityId(3, 1)));
    y_test_assert(set.size() == 64);

    y_test_assert(set.contains(EntityId(9, 1)));
    y_test_assert(!set.contains(EntityId(9, 2)));
    y_test_assert(!set.contains(EntityId(10, 1)));
    y_test_assert(!set.contains(EntityId(100000, 1)));

    y_test_assert(set.id_from_index(9) == EntityId(9, 1));
    y_test_assert(!set.id_from_index(100000).is_valid());
    y_test_assert(set.id_from_index(10).version() == u32(-1));

    y_test_assert(set.erase(EntityId(9, 1)));
    y_test_assert(!set.erase(EntityId(9, 1)));
    y_test_assert(!set.erase(EntityId(12, 2)));
    y_test_assert(!set.contains(EntityId(9, 1)));
    y_test_assert(set.size() == 63);

    // Erase everything in a scrambled order and check consistency on the way
    usize expected = set.size();
    for(u32 i = 0; i != 64; ++i) {
        const EntityId id((i * 37 % 64) * 3, 1);
        if(set.erase(id)) {
            --expected;
        }
        y_test_assert(set.size() == expected);
        y_test_assert(!set.contains(id));
        for(const EntityId other : set) {
            y_test_assert(set.contains(other));
        }
    }
    y_test_assert(set.is_empty());

    set.insert(EntityId(1, 1));
    SparseIdSet moved = std::move(set);
    y_test_assert(moved.contains(EntityId(1, 1)));
    y_test_assert(moved.size() == 1);

    moved.make_empty();
    y_test_assert(moved.is_empty());
    y_test_assert(!moved.contains(EntityId(1, 1)));

    moved.insert(EntityId(2, 1));
    moved.clear();
    y_test_assert(moved.is_empty());
}

y_test_func("Sparse sets with version 0 ids") {
    {
        SparseIdSet set;
        y_test_assert(set.insert(EntityId(4, 0)));
        y_test_assert(set.contains(EntityId(4, 0)));
        y_test_assert(!set.contains(EntityId(4, 1)));

        // Untouched slots must not match version 0
        for(u32 i = 0; i != 4; ++i) {
            y_test_assert(!set.contains(EntityId(i, 0)));
        }

        // Neither should erased ones
        y_test_assert(set.insert(EntityId(0, 0)));
        y_test_assert(set.erase(EntityId(4, 0)));
        y_test_assert(!set.contains(EntityId(4, 0)));
        y_test_assert(set.contains(EntityId(0, 0)));
        y_test_assert(set.size() == 1);
    }

    {
        SparseComponentSet<Position> set;
        set.insert(EntityId(3, 0), 3.0f, 0.0f);
        y_test_assert(set.try_get(EntityId(3, 0))->x == 3.0f);
        for(u32 i = 0; i != 3; ++i) {
            y_test_assert(!set.contains(EntityId(i, 0)));
            y_test_assert(!set.try_get(EntityId(i, 0)));
        }

        set.insert(EntityId(1, 0), 1.0f, 0.0f);
        y_test_assert(!set.try_get(EntityId(0, 0)));
        y_test_assert(!set.try_get(EntityId(2, 0)));
        y_test_assert(set.try_get(EntityId(1, 0))->x == 1.0f);

        // Erase moves the last element into the hole, the old slot must not alias it
        set.erase(EntityId(3, 0));
        y_test_assert(!set.try_get(EntityId(3, 0)));
        y_test_assert(set.try_get(EntityId(1, 0))->x == 1.0f);
        set.erase(EntityId(1, 0));
        y_test_assert(set.is_empty());
        y_test_assert(!set.try_get(EntityId(1, 0)));
    }
}

y_test_func("SparseComponentSet") {
    SparseComponentSet<Position> set;

    for(u32 i = 0; i != 32; ++i) {
        set.insert(EntityId(i, 1), float(i), float(i * 2));
    }
    y_test_assert(set.size() == 32);
    y_test_assert(set.values().size() == 32);

    y_test_assert(set[EntityId(5, 1)].x == 5.0f);
    y_test_assert(set.try_get(EntityId(5, 1))->y == 10.0f);
    y_test_assert(!set.try_get(EntityId(5, 2)));
    y_test_assert(!set.try_get(EntityId(500, 1)));

    // Erasing moves the last element in the hole
    y_test_assert(set.erase(EntityId(5, 1)));
    y_test_assert(!set.erase(EntityId(5, 1)));
    y_test_assert(!set.contains(EntityId(5, 1)));
    y_test_assert(set.size() == 31);
    for(u32 i = 0; i != 32; ++i) {
        if(i != 5) {
            y_test_assert(set[EntityId(i, 1)].x == float(i));
            y_test_assert(set[EntityId(i, 1)].y == float(i * 2));
        }
    }

    // Erase last element
    y_test_assert(set.erase(set.ids()[set.size() - 1]));
    y_test_assert(set.size() == 30);

    Position& existing = set.get_or_insert(EntityId(0, 1), 100.0f, 100.0f);
    y_test_assert(existing.x == 0.0f);
    Position& inserted = set.get_or_insert(EntityId(5, 1), 100.0f, 200.0f);
    y_test_assert(inserted.x == 100.0f);
    y_test_assert(set.size() == 31);

    usize count = 0;
    for(auto&& [id, pos] : set) {
        y_test_assert(set.contains(id));
        y_test_assert(&set[id] == &pos);
        ++count;
    }
    y_test_assert(count == set.size());

    const auto& cset = set;
    y_test_assert(cset.end() - cset.begin() == std::ptrdiff_t(cset.size()));
    y_test_assert(cset.try_get(EntityId(5, 1))->y == 200.0f);

    for(auto&& [id, pos] : set) {
        pos.x = -1.0f;
    }
    for(const Position& p : cset.values()) {
        y_test_assert(p.x == -1.0f);
    }

    set.make_empty();
    y_test_assert(set.is_empty());
    y_test_assert(!set.try_get(EntityId(5, 1)));
}

y_test_func("ComponentRuntimeInfo names") {
    const ComponentRuntimeInfo info = ComponentRuntimeInfo::create<Position>();
    y_test_assert(info.type_id == type_index<Position>());
    y_test_assert(info.clean_component_name() == "Position");
    y_test_assert(!info.is_inspectable);
    y_test_assert(info.required.is_empty());
    y_test_assert(info.create_type_container);
    y_test_assert(info.add_or_replace_component);

    const ComponentRuntimeInfo dep_info = ComponentRuntimeInfo::create<Dependent>();
    y_test_assert(dep_info.required.size() == 1);
    y_test_assert(dep_info.required[0] == type_index<Position>());

    y_test_assert(ComponentRuntimeInfo::create<Named>().is_inspectable);

    std::unique_ptr<ComponentContainerBase> container = info.create_type_container();
    y_test_assert(container);
    y_test_assert(container->type_id() == type_index<Position>());
}

y_test_func("Component type indices") {
    y_test_assert(type_index<Position>() == type_index<Position>());
    y_test_assert(type_index<Position>() != type_index<Velocity>());
    y_test_assert(type_index<Dependent>() != type_index<DeepDependent>());
    y_test_assert(type_index<Position>() != ComponentTypeIndex::invalid_index);
}



// ---------------------------------------- Entities ----------------------------------------

y_test_func("EntityWorld create entities") {
    EntityWorld world;
    y_test_assert(world.entity_count() == 0);
    y_test_assert(world.recently_added().is_empty());
    y_test_assert(world.tick_id() == TickId());

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 100; ++i) {
        const EntityId id = world.create_entity();
        y_test_assert(id.is_valid());
        y_test_assert(world.exists(id));
        y_test_assert(!contains(ids, id));
        ids << id;
    }

    y_test_assert(world.entity_count() == 100);
    y_test_assert(world.entity_pool().size() == 100);
    y_test_assert(same_ids(world.recently_added(), ids));
    y_test_assert(same_ids(collect(world.entity_pool().ids()), ids));

    world.process_deferred_changes();

    y_test_assert(world.entity_count() == 100);
    y_test_assert(world.recently_added().is_empty());
    for(const EntityId id : ids) {
        y_test_assert(world.exists(id));
    }

    y_test_assert(!world.exists(EntityId()));
    y_test_assert(!world.exists(EntityId(1000, 1)));
}

y_test_func("EntityWorld remove entities") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    const EntityId c = world.create_entity();
    world.add_or_replace_component<Position>(b, 1.0f, 2.0f);
    world.process_deferred_changes();

    world.remove_entity(b);
    world.remove_entity(b);

    // Removal is deferred
    y_test_assert(world.exists(b));
    y_test_assert(world.entity_count() == 3);
    y_test_assert(world.pending_deletions().size() == 1);
    y_test_assert(world.pending_deletions().contains(b));
    y_test_assert(world.component<Position>(b));
    y_test_assert(container_of<Position>(world)->pending_deletions().contains(b));

    world.process_deferred_changes();

    y_test_assert(!world.exists(b));
    y_test_assert(world.exists(a));
    y_test_assert(world.exists(c));
    y_test_assert(world.entity_count() == 2);
    y_test_assert(world.pending_deletions().is_empty());
    y_test_assert(!world.component<Position>(b));
    y_test_assert(world.component_set<Position>().is_empty());

    // Index is reused, but with a different version
    const EntityId d = world.create_entity();
    y_test_assert(d.index() == b.index());
    y_test_assert(d.version() != b.version());
    y_test_assert(d != b);
    y_test_assert(world.exists(d));
    y_test_assert(!world.exists(b));
    y_test_assert(!world.component<Position>(d));
    y_test_assert(!world.component<Position>(b));
}

y_test_func("EntityWorld remove all entities") {
    EntityWorld world;

    for(usize i = 0; i != 50; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        if(i % 2) {
            world.add_or_replace_component<Velocity>(id);
        }
        world.add_tag(id, "tag");
    }
    world.process_deferred_changes();

    world.remove_all_entities();
    y_test_assert(world.pending_deletions().size() == 50);
    world.process_deferred_changes();

    y_test_assert(world.entity_count() == 0);
    y_test_assert(world.component_set<Position>().is_empty());
    y_test_assert(world.component_set<Velocity>().is_empty());
    y_test_assert(world.tag_set("tag")->is_empty());

    // World is still usable
    const EntityId id = world.create_entity();
    y_test_assert(world.exists(id));
    world.add_or_replace_component<Position>(id, 4.0f, 5.0f);
    y_test_assert(world.component<Position>(id)->x == 4.0f);
}

y_test_func("EntityWorld create with id") {
    EntityWorld world;

    const EntityId fixed(10, 3);
    y_test_assert(world.create_entity_with_id(fixed) == fixed);
    y_test_assert(world.exists(fixed));
    y_test_assert(!world.exists(EntityId(10, 4)));
    y_test_assert(world.entity_count() == 1);
    y_test_assert(contains(world.recently_added(), fixed));

    // The skipped indices are free and used first
    core::Vector<EntityId> ids;
    for(usize i = 0; i != 10; ++i) {
        const EntityId id = world.create_entity();
        y_test_assert(id.index() < 10);
        y_test_assert(!contains(ids, id));
        ids << id;
    }
    y_test_assert(world.entity_count() == 11);

    const EntityId next = world.create_entity();
    y_test_assert(next.index() == 11);

    // Re-create in a hole
    world.remove_entity(ids[3]);
    world.process_deferred_changes();
    const EntityId hole(ids[3].index(), 42);
    y_test_assert(world.create_entity_with_id(hole) == hole);
    y_test_assert(world.exists(hole));
    y_test_assert(world.entity_count() == 12);

    world.add_or_replace_component<Position>(fixed, 1.0f, 1.0f);
    y_test_assert(world.component<Position>(fixed)->x == 1.0f);
}



// ---------------------------------------- Components ----------------------------------------

y_test_func("EntityWorld add and get components") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();

    Position* pos = world.add_or_replace_component<Position>(a, 1.0f, 2.0f);
    y_test_assert(pos);
    y_test_assert(pos->x == 1.0f && pos->y == 2.0f);
    y_test_assert(world.component<Position>(a) == pos);
    y_test_assert(world.has_component<Position>(a));
    y_test_assert(world.has_component(a, type_index<Position>()));
    y_test_assert(!world.has_component<Velocity>(a));
    y_test_assert(!world.has_component<Position>(b));
    y_test_assert(!world.component<Position>(b));
    y_test_assert(!world.component<Velocity>(a));

    // get_or_add returns the existing component
    y_test_assert(world.get_or_add_component<Position>(a) == pos);
    y_test_assert(pos->x == 1.0f);

    // get_or_add creates a default component
    Velocity* vel = world.get_or_add_component<Velocity>(b);
    y_test_assert(vel);
    y_test_assert(vel->dx == 0.0f);
    y_test_assert(world.has_component<Velocity>(b));

    // replacing keeps the component in place
    Position* replaced = world.add_or_replace_component<Position>(a, 5.0f, 6.0f);
    y_test_assert(world.component<Position>(a)->x == 5.0f);
    y_test_assert(replaced->y == 6.0f);

    // replace with a component value
    world.add_or_replace_component<Position>(a, Position(7.0f, 8.0f));
    y_test_assert(world.component<Position>(a)->x == 7.0f);
    y_test_assert(world.component<Position>(a)->y == 8.0f);

    const Position copy(9.0f, 10.0f);
    world.add_or_replace_component<Position>(b, copy);
    y_test_assert(world.component<Position>(b)->x == 9.0f);

    y_test_assert(world.component_set<Position>().size() == 2);
    y_test_assert(world.component_set<Velocity>().size() == 1);

    world.component_mut<Position>(a)->x = 42.0f;
    y_test_assert(world.component<Position>(a)->x == 42.0f);
    y_test_assert(!world.component_mut<Velocity>(a));

    // Components survive processing
    world.process_deferred_changes();
    y_test_assert(world.component<Position>(a)->x == 42.0f);
    y_test_assert(world.component<Position>(b)->x == 9.0f);

    // Strings
    world.add_or_replace_component<Named>(a, core::String("entity a"), 3u);
    y_test_assert(world.component<Named>(a)->name == "entity a");
    y_test_assert(world.component<Named>(a)->count == 3);
}

y_test_func("EntityWorld component removal is deferred") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    world.add_or_replace_component<Position>(a, 1.0f, 1.0f);
    world.add_or_replace_component<Position>(b, 2.0f, 2.0f);
    world.add_or_replace_component<Velocity>(a, 3.0f);
    world.process_deferred_changes();

    world.remove_component<Position>(a);
    y_test_assert(world.has_component<Position>(a));
    y_test_assert(world.component<Position>(a)->x == 1.0f);
    y_test_assert(container_of<Position>(world)->pending_deletions().contains(a));

    // Removing a component the entity doesn't have does nothing
    world.remove_component<Velocity>(b);
    y_test_assert(!container_of<Velocity>(world)->pending_deletions().contains(b));

    world.process_deferred_changes();

    y_test_assert(!world.has_component<Position>(a));
    y_test_assert(!world.component<Position>(a));
    y_test_assert(world.has_component<Velocity>(a));
    y_test_assert(world.component<Position>(b)->x == 2.0f);
    y_test_assert(container_of<Position>(world)->pending_deletions().is_empty());

    // Runtime type removal
    world.remove_component(a, type_index<Velocity>());
    y_test_assert(world.has_component<Velocity>(a));
    world.process_deferred_changes();
    y_test_assert(!world.has_component<Velocity>(a));
    y_test_assert(world.exists(a));

    // Remove all components
    world.add_or_replace_component<Position>(a);
    world.add_or_replace_component<Velocity>(a);
    world.add_or_replace_component<Named>(a);
    world.process_deferred_changes();
    world.remove_all_components(a);
    world.process_deferred_changes();
    y_test_assert(world.exists(a));
    y_test_assert(!world.has_component<Position>(a));
    y_test_assert(!world.has_component<Velocity>(a));
    y_test_assert(!world.has_component<Named>(a));
    y_test_assert(world.has_component<Position>(b));
}

y_test_func("EntityWorld add_or_replace cancels pending removal") {
    EntityWorld world;

    const EntityId id = world.create_entity();
    world.add_or_replace_component<Position>(id, 1.0f, 1.0f);
    world.process_deferred_changes();

    world.remove_component<Position>(id);
    world.add_or_replace_component<Position>(id, 2.0f, 2.0f);
    y_test_assert(!container_of<Position>(world)->pending_deletions().contains(id));

    world.process_deferred_changes();
    y_test_assert(world.has_component<Position>(id));
    y_test_assert(world.component<Position>(id)->x == 2.0f);
}

y_test_func("EntityWorld readd component after removal") {
    EntityWorld world;

    const EntityId id = world.create_entity();
    world.add_or_replace_component<Position>(id, 1.0f, 1.0f);
    world.process_deferred_changes();

    world.remove_component<Position>(id);
    world.process_deferred_changes();
    y_test_assert(!world.has_component<Position>(id));

    world.add_or_replace_component<Position>(id, 3.0f, 3.0f);
    y_test_assert(world.has_component<Position>(id));
    world.process_deferred_changes();
    y_test_assert(world.component<Position>(id)->x == 3.0f);
}

y_test_func("EntityWorld mutation tracking") {
    EntityWorld world;
    const ComponentContainerBase* positions = container_of<Position>(world);
    y_test_assert(positions);

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();

    world.add_or_replace_component<Position>(a);
    y_test_assert(positions->mutated_ids().contains(a));
    world.get_or_add_component<Position>(b);
    y_test_assert(positions->mutated_ids().contains(b));

    world.process_deferred_changes();
    y_test_assert(positions->mutated_ids().is_empty());

    // const access doesn't mark mutation
    y_test_assert(world.component<Position>(a));
    y_test_assert(world.get_or_add_component<Position>(a));
    y_test_assert(positions->mutated_ids().is_empty());

    world.component_mut<Position>(a);
    y_test_assert(positions->mutated_ids().contains(a));
    y_test_assert(!positions->mutated_ids().contains(b));

    // Missing components are not marked
    world.component_mut<Velocity>(b);
    y_test_assert(container_of<Velocity>(world)->mutated_ids().is_empty());

    world.add_or_replace_component<Position>(b, 1.0f, 1.0f);
    y_test_assert(positions->mutated_ids().contains(b));

    world.process_deferred_changes();
    y_test_assert(positions->mutated_ids().is_empty());
}

y_test_func("EntityWorld required components") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    world.add_or_replace_component<Dependent>(a, 7u);
    y_test_assert(world.has_component<Dependent>(a));
    y_test_assert(world.has_component<Position>(a));
    y_test_assert(world.component<Dependent>(a)->value == 7);
    y_test_assert(world.is_component_required(a, type_index<Position>()));
    y_test_assert(!world.is_component_required(a, type_index<Dependent>()));

    // Existing required components are kept as is
    const EntityId b = world.create_entity();
    world.add_or_replace_component<Position>(b, 3.0f, 4.0f);
    world.add_or_replace_component<Dependent>(b);
    y_test_assert(world.component<Position>(b)->x == 3.0f);

    // Transitive requirements
    const EntityId c = world.create_entity();
    world.add_or_replace_component<DeepDependent>(c);
    y_test_assert(world.has_component<DeepDependent>(c));
    y_test_assert(world.has_component<Dependent>(c));
    y_test_assert(world.has_component<Position>(c));
    y_test_assert(world.is_component_required(c, type_index<Dependent>()));
    y_test_assert(world.is_component_required(c, type_index<Position>()));

    world.process_deferred_changes();

    // Required components can't be removed
    world.remove_component<Position>(a);
    world.process_deferred_changes();
    y_test_assert(world.has_component<Position>(a));
    y_test_assert(world.has_component<Dependent>(a));

    // Unless what requires them is also removed
    world.remove_component<Position>(a);
    world.remove_component<Dependent>(a);
    world.process_deferred_changes();
    y_test_assert(!world.has_component<Position>(a));
    y_test_assert(!world.has_component<Dependent>(a));

    // Whole chain at once
    world.remove_all_components(c);
    world.process_deferred_changes();
    y_test_assert(!world.has_component<DeepDependent>(c));
    y_test_assert(!world.has_component<Dependent>(c));
    y_test_assert(!world.has_component<Position>(c));

    // Removing the dependent lets the required component be removed later
    world.remove_component<Dependent>(b);
    world.process_deferred_changes();
    y_test_assert(!world.has_component<Dependent>(b));
    y_test_assert(world.has_component<Position>(b));
    y_test_assert(!world.is_component_required(b, type_index<Position>()));
    world.remove_component<Position>(b);
    world.process_deferred_changes();
    y_test_assert(!world.has_component<Position>(b));

    // Removing the entity removes everything
    const EntityId d = world.create_entity();
    world.add_or_replace_component<DeepDependent>(d);
    world.process_deferred_changes();
    world.remove_entity(d);
    world.process_deferred_changes();
    y_test_assert(!world.exists(d));
    y_test_assert(world.component_set<DeepDependent>().is_empty());
    y_test_assert(world.component_set<Dependent>().is_empty());
    y_test_assert(world.component_set<Position>().is_empty());
}

y_test_func("EntityWorld component containers") {
    EntityWorld world;

    const auto containers = world.component_containers();
    y_test_assert(!containers.is_empty());

    const ComponentContainerBase* pos = container_of<Position>(world);
    const ComponentContainerBase* dep = container_of<Dependent>(world);
    const ComponentContainerBase* deep = container_of<DeepDependent>(world);
    const ComponentContainerBase* named = container_of<Named>(world);
    y_test_assert(pos && dep && deep && named);
    y_test_assert(container_of<Velocity>(world));
    y_test_assert(container_of<Link>(world));
    y_test_assert(container_of<Registered>(world));

    y_test_assert(pos->requirements_chain_depth() == 0);
    y_test_assert(dep->requirements_chain_depth() == 1);
    y_test_assert(deep->requirements_chain_depth() == 2);

    // Containers are sorted by requirement depth so dependents get processed first
    for(usize i = 1; i < containers.size(); ++i) {
        y_test_assert(containers[i - 1]->requirements_chain_depth() >= containers[i]->requirements_chain_depth());
    }

    // No duplicates
    for(usize i = 0; i != containers.size(); ++i) {
        for(usize j = i + 1; j != containers.size(); ++j) {
            y_test_assert(containers[i]->type_id() != containers[j]->type_id());
        }
    }

    y_test_assert(world.component_type_name(type_index<Position>()) == "Position");
    y_test_assert(world.component_type_name(type_index<DeepDependent>()) == "DeepDependent");

    y_test_assert(pos->runtime_info().type_id == type_index<Position>());
    y_test_assert(!pos->runtime_info().is_inspectable);
    y_test_assert(named->runtime_info().is_inspectable);
    y_test_assert(dep->runtime_info().required.size() == 1);
}



// ---------------------------------------- Tags ----------------------------------------

y_test_func("EntityWorld tags") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    const EntityId c = world.create_entity();

    y_test_assert(!world.tag_set("red"));
    y_test_assert(!world.has_tag(a, "red"));

    world.add_tag(a, "red");
    world.add_tag(a, "red");
    world.add_tag(b, "red");
    world.add_tag(b, "blue");
    world.add_tag(c, core::String(tags::hidden));

    y_test_assert(world.has_tag(a, "red"));
    y_test_assert(world.has_tag(b, "red"));
    y_test_assert(world.has_tag(b, "blue"));
    y_test_assert(!world.has_tag(a, "blue"));
    y_test_assert(!world.has_tag(c, "red"));
    y_test_assert(world.has_tag(c, core::String(tags::hidden)));

    y_test_assert(world.tag_set("red"));
    y_test_assert(world.tag_set("red")->size() == 2);
    y_test_assert(world.tag_set("blue")->size() == 1);

    usize tag_count = 0;
    bool found_red = false;
    for(const core::String& tag : world.tags()) {
        found_red |= (tag == "red");
        ++tag_count;
    }
    y_test_assert(found_red);
    y_test_assert(tag_count == 3);

    world.remove_tag(a, "red");
    y_test_assert(!world.has_tag(a, "red"));
    y_test_assert(world.tag_set("red")->size() == 1);

    // removing a tag that isn't there is fine
    world.remove_tag(a, "red");
    world.remove_tag(a, "never_added");
    y_test_assert(world.tag_set("red")->size() == 1);

    world.add_tag(a, "red");
    world.clear_tag("red");
    y_test_assert(world.tag_set("red")->is_empty());
    y_test_assert(!world.has_tag(a, "red"));
    y_test_assert(!world.has_tag(b, "red"));
    y_test_assert(world.has_tag(b, "blue"));

    // Tags are removed immediately when the entity is removed
    world.remove_entity(b);
    y_test_assert(world.tag_set("blue")->is_empty());
    world.process_deferred_changes();

    world.add_tag(a, "blue");
    world.remove_all_tags(a);
    y_test_assert(!world.has_tag(a, "blue"));
    y_test_assert(world.has_tag(c, core::String(tags::hidden)));
}

y_test_func("Computed tags") {
    y_test_assert(is_computed_tag(tags::not_hidden));
    y_test_assert(is_computed_tag("@foo"));
    y_test_assert(!is_computed_tag(tags::hidden));
    y_test_assert(!is_computed_tag(""));
    y_test_assert(is_computed_not_tag(tags::not_selected));
    y_test_assert(!is_computed_not_tag("@foo"));
    y_test_assert(raw_tag(tags::not_debug) == tags::debug);
    y_test_assert(raw_tag("@foo") == "foo");
    y_test_assert(raw_tag("foo") == "foo");
}



// ---------------------------------------- Hierarchy ----------------------------------------

y_test_func("EntityWorld hierarchy basics") {
    EntityWorld world;

    const EntityId root = world.create_entity();
    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    const EntityId c = world.create_entity();
    const EntityId grand = world.create_entity();
    world.process_deferred_changes();

    y_test_assert(!world.has_parent(root));
    y_test_assert(!world.has_children(root));
    y_test_assert(!world.parent(root).is_valid());
    y_test_assert(collect(world.direct_children(root)).is_empty());
    y_test_assert(collect(world.parents(root)).is_empty());

    world.set_parent(a, root);
    world.set_parent(b, root);
    world.set_parent(c, root);
    world.set_parent(grand, b);

    y_test_assert(world.parent_changed().size() == 4);
    y_test_assert(world.parent_changed().contains(a));
    y_test_assert(world.parent_changed().contains(grand));
    y_test_assert(!world.parent_changed().contains(root));

    y_test_assert(world.parent(a) == root);
    y_test_assert(world.parent(grand) == b);
    y_test_assert(world.has_parent(a));
    y_test_assert(world.has_children(root));
    y_test_assert(world.has_children(b));
    y_test_assert(!world.has_children(a));

    // Children are kept in insertion order
    const core::Vector<EntityId> children = collect(world.direct_children(root));
    y_test_assert(children.size() == 3);
    y_test_assert(children[0] == a);
    y_test_assert(children[1] == b);
    y_test_assert(children[2] == c);

    const core::Vector<EntityId> parents = collect(world.parents(grand));
    y_test_assert(parents.size() == 2);
    y_test_assert(parents[0] == b);
    y_test_assert(parents[1] == root);

    y_test_assert(world.is_parent(grand, b));
    y_test_assert(world.is_parent(grand, root));
    y_test_assert(!world.is_parent(grand, a));
    y_test_assert(!world.is_parent(root, grand));
    y_test_assert(!world.is_parent(root, root));
    y_test_assert(!world.is_parent(a, EntityId()));
    y_test_assert(!world.is_parent(EntityId(), root));

    world.process_deferred_changes();
    y_test_assert(world.parent_changed().is_empty());

    // Self parenting is ignored
    world.set_parent(a, a);
    y_test_assert(world.parent(a) == root);
    y_test_assert(world.parent_changed().is_empty());

    // Setting the same parent is a no-op on the hierarchy
    world.set_parent(a, root);
    y_test_assert(collect(world.direct_children(root)).size() == 3);
}

y_test_func("EntityWorld reparenting") {
    EntityWorld world;

    const EntityId p1 = world.create_entity();
    const EntityId p2 = world.create_entity();
    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    const EntityId c = world.create_entity();

    world.set_parent(a, p1);
    world.set_parent(b, p1);
    world.set_parent(c, p1);

    // Move middle child
    world.set_parent(b, p2);
    y_test_assert(world.parent(b) == p2);
    {
        const auto c1 = collect(world.direct_children(p1));
        y_test_assert(c1.size() == 2);
        y_test_assert(c1[0] == a && c1[1] == c);
        const auto c2 = collect(world.direct_children(p2));
        y_test_assert(c2.size() == 1 && c2[0] == b);
    }

    // Move first child
    world.set_parent(a, p2);
    {
        const auto c1 = collect(world.direct_children(p1));
        y_test_assert(c1.size() == 1 && c1[0] == c);
        const auto c2 = collect(world.direct_children(p2));
        y_test_assert(c2.size() == 2 && c2[0] == b && c2[1] == a);
    }

    // Move only child
    world.set_parent(c, p2);
    y_test_assert(!world.has_children(p1));
    y_test_assert(collect(world.direct_children(p2)).size() == 3);

    // Unparent
    world.set_parent(b, EntityId());
    y_test_assert(!world.has_parent(b));
    {
        const auto c2 = collect(world.direct_children(p2));
        y_test_assert(c2.size() == 2 && c2[0] == a && c2[1] == c);
    }

    // Nest under a sibling
    world.set_parent(c, a);
    y_test_assert(world.is_parent(c, p2));
    y_test_assert(collect(world.direct_children(p2)).size() == 1);

    // Move a whole subtree
    world.set_parent(p2, p1);
    y_test_assert(world.is_parent(c, p1));
    y_test_assert(collect(world.parents(c)).size() == 3);

    world.process_deferred_changes();
}

y_test_func("EntityWorld removing parent reroots children") {
    EntityWorld world;

    const EntityId grand = world.create_entity();
    const EntityId parent = world.create_entity();
    const EntityId c1 = world.create_entity();
    const EntityId c2 = world.create_entity();
    const EntityId other = world.create_entity();

    world.set_parent(parent, grand);
    world.set_parent(other, grand);
    world.set_parent(c1, parent);
    world.set_parent(c2, parent);
    world.process_deferred_changes();

    world.remove_entity(parent);
    // Nothing changes until changes are processed
    y_test_assert(world.parent(c1) == parent);
    world.process_deferred_changes();

    y_test_assert(!world.exists(parent));
    y_test_assert(world.parent(c1) == grand);
    y_test_assert(world.parent(c2) == grand);

    const auto children = collect(world.direct_children(grand));
    y_test_assert(children.size() == 3);
    y_test_assert(contains(children, other));
    y_test_assert(contains(children, c1));
    y_test_assert(contains(children, c2));

    // Removing a root makes its children roots
    world.remove_entity(grand);
    world.process_deferred_changes();
    y_test_assert(!world.has_parent(c1));
    y_test_assert(!world.has_parent(c2));
    y_test_assert(!world.has_parent(other));
    y_test_assert(world.entity_count() == 3);

    // Removing a leaf
    world.set_parent(c2, c1);
    world.process_deferred_changes();
    world.remove_entity(c2);
    world.process_deferred_changes();
    y_test_assert(!world.has_children(c1));
}



// ---------------------------------------- Groups ----------------------------------------

y_test_func("EntityWorld groups") {
    EntityWorld world;

    core::Vector<EntityId> with_pos;
    core::Vector<EntityId> with_both;
    for(u32 i = 0; i != 30; ++i) {
        const EntityId id = world.create_entity();
        if(i % 2 == 0) {
            world.add_or_replace_component<Position>(id, float(i), 0.0f);
            with_pos << id;
        }
        if(i % 3 == 0) {
            world.add_or_replace_component<Velocity>(id, float(i));
            if(i % 2 == 0) {
                with_both << id;
            }
        }
    }
    world.process_deferred_changes();

    {
        auto group = world.create_group<Position>();
        y_test_assert(group.size() == with_pos.size());
        y_test_assert(!group.is_empty());
        y_test_assert(same_ids(group.ids(), with_pos));

        float sum = 0.0f;
        for(auto&& [pos] : group) {
            sum += pos.x;
        }
        y_test_assert(sum == 0.0f + 2 + 4 + 6 + 8 + 10 + 12 + 14 + 16 + 18 + 20 + 22 + 24 + 26 + 28);

        for(auto&& [id, pos] : group.id_components()) {
            y_test_assert(world.component<Position>(id) == &pos);
        }
    }

    {
        auto group = world.create_group<Position, Velocity>();
        y_test_assert(same_ids(group.ids(), with_both));
        for(auto&& [id, pos, vel] : group.id_components()) {
            y_test_assert(pos.x == vel.dx);
        }
    }

    {
        auto group = world.create_group<Velocity, Position>();
        y_test_assert(same_ids(group.ids(), with_both));
    }

    {
        auto group = world.create_group<Link>();
        y_test_assert(group.is_empty());
        y_test_assert(group.begin() == group.end());
    }

    // Providers are shared
    const EntityGroupProvider* provider = world.get_or_create_group_provider<Position>();
    y_test_assert(provider == world.get_or_create_group_provider<Position>());
    y_test_assert((provider != world.get_or_create_group_provider<Position, Velocity>()));
    y_test_assert(provider->types().size() == 1);
    y_test_assert(provider->types()[0] == type_index<Position>());
    y_test_assert(provider->tags().is_empty());
    y_test_assert(provider->type_filters().is_empty());
    y_test_assert(provider->ids().size() == with_pos.size());
    y_test_assert(!provider->name().is_empty());

    {
        auto group = world.create_group<Position>(provider);
        y_test_assert(group.base() == provider);
        y_test_assert(group.size() == with_pos.size());
    }

    const auto providers = world.group_providers();
    y_test_assert(std::find(providers.begin(), providers.end(), provider) != providers.end());
    const usize provider_count = providers.size();
    world.create_group<Position>();
    y_test_assert(world.group_providers().size() == provider_count);
}

y_test_func("EntityWorld groups update incrementally") {
    EntityWorld world;

    const EntityGroupProvider* provider = world.get_or_create_group_provider<Position, Velocity>();
    y_test_assert(provider->ids().is_empty());

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();

    world.add_or_replace_component<Position>(a);
    y_test_assert(!provider->ids().contains(a));
    world.add_or_replace_component<Velocity>(a);
    y_test_assert(provider->ids().contains(a));
    y_test_assert(provider->added_ids().contains(a));

    world.add_or_replace_component<Velocity>(b);
    y_test_assert(!provider->ids().contains(b));

    world.process_deferred_changes();
    y_test_assert(provider->ids().contains(a));
    y_test_assert(provider->added_ids().is_empty());
    y_test_assert(provider->removed_ids().is_empty());

    world.remove_component<Velocity>(a);
    y_test_assert(provider->ids().contains(a));
    world.process_deferred_changes();
    y_test_assert(!provider->ids().contains(a));
    y_test_assert(provider->removed_ids().contains(a));

    // removed ids are reset on the next process
    world.process_deferred_changes();
    y_test_assert(provider->removed_ids().is_empty());

    // Re-adding
    world.add_or_replace_component<Velocity>(a);
    y_test_assert(provider->ids().contains(a));
    world.process_deferred_changes();

    // Entity removal
    world.remove_entity(a);
    world.process_deferred_changes();
    y_test_assert(provider->ids().is_empty());
    y_test_assert(provider->removed_ids().contains(a));

    world.add_or_replace_component<Position>(b);
    y_test_assert(provider->ids().contains(b));
    y_test_assert((world.create_group<Position, Velocity>().size() == 1));
}

y_test_func("EntityWorld Mutate groups") {
    EntityWorld world;

    for(usize i = 0; i != 10; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        if(i % 2) {
            world.add_or_replace_component<Velocity>(id, 1.0f);
        }
    }
    world.process_deferred_changes();

    const ComponentContainerBase* positions = container_of<Position>(world);
    const ComponentContainerBase* velocities = container_of<Velocity>(world);

    {
        auto group = world.create_group<Mutate<Position>, Velocity>();
        y_test_assert(group.size() == 5);
        for(auto&& [pos, vel] : group) {
            pos.x += vel.dx;
        }
    }

    y_test_assert(positions->mutated_ids().size() == 5);
    y_test_assert(velocities->mutated_ids().is_empty());

    for(auto&& [id, pos] : world.component_set<Position>()) {
        y_test_assert(pos.x == (world.has_component<Velocity>(id) ? 1.0f : 0.0f));
        y_test_assert(positions->mutated_ids().contains(id) == world.has_component<Velocity>(id));
    }
}

y_test_func("EntityWorld MutateUntracked groups") {
    EntityWorld world;

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 10; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        world.add_or_replace_component<Velocity>(id);
        ids << id;
    }
    world.process_deferred_changes();

    const ComponentContainerBase* positions = container_of<Position>(world);
    const ComponentContainerBase* velocities = container_of<Velocity>(world);

    {
        auto group = world.create_group<MutateUntracked<Position>, Mutate<Velocity>>();
        y_test_assert(group.size() == 10);
        for(auto&& [id, pos, vel] : group.id_components()) {
            if(id == ids[3]) {
                pos.x = 1.0f;
                group.mark_changed<Position>(id);
            }
        }
    }

    // Only the explicitly marked position is changed, Mutate still marks everything
    y_test_assert(positions->mutated_ids().size() == 1);
    y_test_assert(positions->mutated_ids().contains(ids[3]));
    y_test_assert(velocities->mutated_ids().size() == 10);
    y_test_assert(world.component<Position>(ids[3])->x == 1.0f);

    {
        auto group = world.create_group<Changed<Position>>();
        y_test_assert(group.size() == 1);
        y_test_assert(group.ids()[0] == ids[3]);
    }
}

y_test_func("EntityWorld Changed groups") {
    EntityWorld world;

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 10; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        world.add_or_replace_component<Velocity>(id);
        ids << id;
    }

    // Newly added components count as changed
    y_test_assert(world.create_group<Changed<Position>>().size() == 10);

    world.process_deferred_changes();
    y_test_assert(world.create_group<Changed<Position>>().is_empty());

    world.component_mut<Position>(ids[2]);
    world.component_mut<Position>(ids[5]);
    world.component_mut<Velocity>(ids[7]);

    {
        auto group = world.create_group<Changed<Position>>();
        y_test_assert(group.size() == 2);
        y_test_assert(contains(group.ids(), ids[2]));
        y_test_assert(contains(group.ids(), ids[5]));
    }

    {
        auto group = world.create_group<Changed<Position>, Changed<Velocity>>();
        y_test_assert(group.is_empty());
    }

    world.component_mut<Velocity>(ids[2]);
    {
        auto group = world.create_group<Changed<Position>, Changed<Velocity>>();
        y_test_assert(group.size() == 1);
        y_test_assert(group.ids()[0] == ids[2]);
    }

    {
        auto group = world.create_group<Changed<Position>, Velocity>();
        y_test_assert(group.size() == 2);
    }

    {
        auto group = world.create_group<Mutate<Changed<Velocity>>>();
        y_test_assert(group.size() == 2);
        for(auto&& [vel] : group) {
            vel.dx = 5.0f;
        }
    }
    y_test_assert(world.component<Velocity>(ids[7])->dx == 5.0f);
    y_test_assert(world.component<Velocity>(ids[0])->dx == 0.0f);
}

y_test_func("EntityWorld AnyChanged groups") {
    EntityWorld world;

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 10; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        if(i < 8) {
            world.add_or_replace_component<Velocity>(id);
        }
        ids << id;
    }
    world.process_deferred_changes();

    y_test_assert((world.create_group<AnyChanged<Position>, AnyChanged<Velocity>>().is_empty()));

    world.component_mut<Position>(ids[1]);
    world.component_mut<Velocity>(ids[3]);
    world.component_mut<Velocity>(ids[1]);
    world.component_mut<Position>(ids[9]); // No velocity

    {
        auto group = world.create_group<AnyChanged<Position>, AnyChanged<Velocity>>();
        y_test_assert(group.size() == 2);
        y_test_assert(contains(group.ids(), ids[1]));
        y_test_assert(contains(group.ids(), ids[3]));
    }

    {
        auto group = world.create_group<AnyChanged<Position>>();
        y_test_assert(group.size() == 2);
        y_test_assert(contains(group.ids(), ids[1]));
        y_test_assert(contains(group.ids(), ids[9]));
    }
}

y_test_func("EntityWorld Deleted groups") {
    EntityWorld world;

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 10; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id, float(i), 0.0f);
        ids << id;
    }
    world.process_deferred_changes();

    y_test_assert(world.create_group<Deleted<Position>>().is_empty());

    world.remove_component<Position>(ids[4]);
    world.remove_entity(ids[6]);

    {
        auto group = world.create_group<Deleted<Position>>();
        y_test_assert(group.size() == 2);
        // Deleted components are still accessible
        for(auto&& [id, pos] : group.id_components()) {
            y_test_assert(id == ids[4] || id == ids[6]);
            y_test_assert(pos.x == float(id == ids[4] ? 4 : 6));
        }
    }

    world.process_deferred_changes();
    y_test_assert(world.create_group<Deleted<Position>>().is_empty());
    y_test_assert(world.create_group<Position>().size() == 8);
}

y_test_func("EntityWorld tag groups") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    const EntityId c = world.create_entity();
    world.add_or_replace_component<Position>(a);
    world.add_or_replace_component<Position>(b);
    world.add_tag(a, "red");
    world.add_tag(c, "red");
    world.process_deferred_changes();

    std::array<std::string_view, 1> red = {"red"};
    std::array<std::string_view, 2> red_blue = {"red", "blue"};

    // Group created after tags
    {
        auto group = world.create_group<Position>(red);
        y_test_assert(group.size() == 1);
        y_test_assert(group.ids()[0] == a);
    }

    const EntityGroupProvider* provider = world.get_or_create_group_provider<Position>(red);
    y_test_assert(provider != world.get_or_create_group_provider<Position>());
    y_test_assert(provider == world.get_or_create_group_provider<Position>(red));
    y_test_assert(provider->tags().size() == 1);
    y_test_assert(provider->tags()[0] == "red");

    world.add_tag(b, "red");
    y_test_assert(provider->ids().contains(b));

    world.remove_tag(a, "red");
    y_test_assert(!provider->ids().contains(a));
    y_test_assert(provider->removed_ids().contains(a));

    world.add_or_replace_component<Position>(c);
    y_test_assert(provider->ids().contains(c));

    world.clear_tag("red");
    y_test_assert(provider->ids().is_empty());

    // Multiple tags
    const EntityGroupProvider* multi = world.get_or_create_group_provider<Position>(red_blue);
    world.add_tag(a, "red");
    y_test_assert(!multi->ids().contains(a));
    world.add_tag(a, "blue");
    y_test_assert(multi->ids().contains(a));
    y_test_assert(world.create_group<Position>(red_blue).size() == 1);

    // Removing the entity removes it from tag groups
    world.remove_entity(a);
    y_test_assert(!multi->ids().contains(a));
    world.process_deferred_changes();
    y_test_assert(multi->ids().is_empty());
}

y_test_func("EntityWorld group type filters") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    world.add_or_replace_component<Position>(a);
    world.add_or_replace_component<Position>(b);
    world.add_or_replace_component<Named>(a);
    world.process_deferred_changes();

    std::array<ComponentTypeIndex, 1> filters = {type_index<Named>()};

    {
        auto group = world.create_group<Position>({}, filters);
        y_test_assert(group.size() == 1);
        y_test_assert(group.ids()[0] == a);
    }

    const EntityGroupProvider* provider = world.get_or_create_group_provider<Position>({}, filters);
    y_test_assert(provider->type_filters().size() == 1);
    y_test_assert(provider != world.get_or_create_group_provider<Position>());

    world.add_or_replace_component<Named>(b);
    y_test_assert(provider->ids().contains(b));
    world.remove_component<Named>(a);
    world.process_deferred_changes();
    y_test_assert(!provider->ids().contains(a));
}

y_test_func("EntityWorld const groups") {
    EntityWorld world;

    for(usize i = 0; i != 5; ++i) {
        world.add_or_replace_component<Position>(world.create_entity(), 1.0f, 2.0f);
    }
    world.process_deferred_changes();

    const EntityWorld& cworld = world;
    auto group = cworld.create_group<Position>();
    y_test_assert(group.size() == 5);
    for(auto&& [pos] : group) {
        y_test_assert(pos.y == 2.0f);
    }

    y_test_assert(cworld.get_or_create_group_provider<Position>() == world.get_or_create_group_provider<Position>());
}

y_test_func("EntityWorld groups mixing tags, filters and qualifiers") {
    EntityWorld world;

    core::Vector<EntityId> ids;
    for(usize i = 0; i != 12; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        world.add_or_replace_component<Velocity>(id);
        if(i % 2) {
            world.add_tag(id, "odd");
        }
        if(i % 3 == 0) {
            world.add_or_replace_component<Named>(id);
        }
        ids << id;
    }
    world.process_deferred_changes();

    std::array<std::string_view, 1> odd = {"odd"};
    std::array<ComponentTypeIndex, 1> named = {type_index<Named>()};

    // odd & multiple of 3: 3, 9
    y_test_assert(world.create_group<Position>(odd, named).size() == 2);

    world.component_mut<Position>(ids[3]);
    world.component_mut<Position>(ids[4]);
    {
        auto group = world.create_group<Changed<Position>, Mutate<Velocity>>(odd, named);
        y_test_assert(group.size() == 1);
        y_test_assert(group.ids()[0] == ids[3]);
    }
}



// ---------------------------------------- Prefabs and boxes ----------------------------------------

y_test_func("EntityWorld component boxes") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    world.add_or_replace_component<Position>(a, 1.0f, 2.0f);
    world.add_or_replace_component<Named>(a, core::String("boxed"), 4u);

    y_test_assert(!world.create_box_from_component(b, type_index<Position>()));

    std::unique_ptr<ComponentBoxBase> box = world.create_box_from_component(a, type_index<Position>());
    y_test_assert(box);
    y_test_assert(box->runtime_info().type_id == type_index<Position>());
    y_test_assert(dynamic_cast<ComponentBox<Position>*>(box.get()));
    y_test_assert(dynamic_cast<ComponentBox<Position>*>(box.get())->component().y == 2.0f);

    box->add_or_replace(world, b);
    y_test_assert(world.component<Position>(b)->x == 1.0f);

    // Boxes are copies
    world.component_mut<Position>(a)->x = 10.0f;
    box->add_or_replace(world, b);
    y_test_assert(world.component<Position>(b)->x == 1.0f);

    std::unique_ptr<ComponentBoxBase> named = world.create_box_from_component(a, type_index<Named>());
    named->add_or_replace(world, b);
    y_test_assert(world.component<Named>(b)->name == "boxed");
    y_test_assert(world.component<Named>(b)->count == 4);

    // Boxes can be serialized
    io2::Buffer buffer;
    serde3::WritableArchive(buffer).serialize(named).expected("Unable to serialize box");
    buffer.reset();
    std::unique_ptr<ComponentBoxBase> loaded;
    serde3::ReadableArchive(buffer).deserialize(loaded).expected("Unable to deserialize box");
    y_test_assert(loaded);
    y_test_assert(loaded->runtime_info().type_id == type_index<Named>());

    const EntityId c = world.create_entity();
    loaded->add_or_replace(world, c);
    y_test_assert(world.component<Named>(c)->name == "boxed");

    // Runtime info can create components
    const EntityId d = world.create_entity();
    ComponentRuntimeInfo::create<Velocity>().add_or_replace_component(world, d);
    y_test_assert(world.has_component<Velocity>(d));
}

y_test_func("EntityWorld prefab round trip") {
    EntityWorld world;

    const EntityId outside = world.create_entity();

    const EntityId root = world.create_entity();
    const EntityId child_a = world.create_entity();
    const EntityId child_b = world.create_entity();
    const EntityId grand = world.create_entity();

    world.set_parent(child_a, root);
    world.set_parent(child_b, root);
    world.set_parent(grand, child_a);

    world.add_or_replace_component<Position>(root, 1.0f, 2.0f);
    world.add_or_replace_component<Named>(root, core::String("root"));
    world.add_or_replace_component<Velocity>(child_a, 3.0f);
    world.add_or_replace_component<Link>(child_a, child_b);
    world.add_or_replace_component<Link>(child_b, outside);
    world.add_or_replace_component<Dependent>(grand, 9u);
    world.process_deferred_changes();

    const EntityPrefab prefab = world.create_prefab_from_entity(root);
    y_test_assert(!prefab.is_empty());
    y_test_assert(prefab.original_id() == root);
    y_test_assert(prefab.components().size() == 2);
    y_test_assert(prefab.children().size() == 2);
    y_test_assert(prefab.asset_children().is_empty());

    const EntityId copy = world.create_entity(prefab);
    y_test_assert(copy != root);
    y_test_assert(world.exists(copy));
    y_test_assert(world.entity_count() == 5 + 4);

    y_test_assert(world.component<Position>(copy)->y == 2.0f);
    y_test_assert(world.component<Named>(copy)->name == "root");

    const auto children = collect(world.direct_children(copy));
    y_test_assert(children.size() == 2);

    const EntityId copy_a = world.has_component<Velocity>(children[0]) ? children[0] : children[1];
    const EntityId copy_b = copy_a == children[0] ? children[1] : children[0];
    y_test_assert(copy_a != child_a && copy_b != child_b);
    y_test_assert(world.component<Velocity>(copy_a)->dx == 3.0f);

    // Links inside the prefab get remapped, outside ones are left alone
    y_test_assert(world.component<Link>(copy_a)->target == copy_b);
    y_test_assert(world.component<Link>(copy_b)->target == outside);

    const auto grand_children = collect(world.direct_children(copy_a));
    y_test_assert(grand_children.size() == 1);
    y_test_assert(grand_children[0] != grand);
    y_test_assert(world.component<Dependent>(grand_children[0])->value == 9);
    y_test_assert(world.has_component<Position>(grand_children[0]));
    y_test_assert(world.is_parent(grand_children[0], copy));

    // Original untouched
    y_test_assert(world.component<Link>(child_a)->target == child_b);
    y_test_assert(collect(world.direct_children(root)).size() == 2);

    world.process_deferred_changes();
}

y_test_func("EntityWorld prefab keep ids") {
    EntityWorld world;

    const EntityId root = world.create_entity();
    const EntityId child = world.create_entity();
    world.set_parent(child, root);
    world.add_or_replace_component<Position>(root, 5.0f, 5.0f);
    world.add_or_replace_component<Link>(child, root);

    const EntityPrefab prefab = world.create_prefab_from_entity(root);

    // Make sure ids don't just happen to match
    EntityWorld other;
    const EntityId o0 = other.create_entity();
    const EntityId o1 = other.create_entity();
    const EntityId o2 = other.create_entity();
    other.remove_entity(o0);
    other.remove_entity(o1);
    other.process_deferred_changes();

    y_test_assert(o0.index() == root.index());
    y_test_assert(!other.exists(root));
    y_test_assert(!other.exists(child));
    y_test_assert(other.exists(o2));

    const EntityId copy = other.create_entity(prefab, true);
    y_test_assert(copy == root);
    y_test_assert(other.exists(root));
    y_test_assert(other.exists(child));
    y_test_assert(other.parent(child) == root);
    y_test_assert(other.component<Position>(root)->x == 5.0f);
    y_test_assert(other.component<Link>(child)->target == root);
}

y_test_func("EntityWorld add prefab to existing entity") {
    EntityWorld world;

    EntityPrefab prefab(EntityId(1000, 1));
    prefab.add(Position(3.0f, 4.0f));
    prefab.add(Velocity(7.0f));

    auto child = std::make_unique<EntityPrefab>(EntityId(1001, 1));
    child->add(Link(EntityId(1000, 1)));
    prefab.add_child(std::move(child));

    y_test_assert(prefab.components().size() == 2);
    y_test_assert(prefab.children().size() == 1);

    const EntityId target = world.create_entity();
    world.add_or_replace_component<Position>(target, 0.0f, 0.0f);
    world.add_or_replace_component<Named>(target, core::String("kept"));

    world.add_prefab(target, prefab);

    y_test_assert(world.component<Position>(target)->x == 3.0f);
    y_test_assert(world.component<Velocity>(target)->dx == 7.0f);
    y_test_assert(world.component<Named>(target)->name == "kept");

    const auto children = collect(world.direct_children(target));
    y_test_assert(children.size() == 1);
    y_test_assert(world.component<Link>(children[0])->target == target);

    // Instantiating several times creates distinct entities
    const EntityId i1 = world.create_entity(prefab);
    const EntityId i2 = world.create_entity(prefab);
    y_test_assert(i1 != i2);
    y_test_assert(world.component<Link>(collect(world.direct_children(i1))[0])->target == i1);
    y_test_assert(world.component<Link>(collect(world.direct_children(i2))[0])->target == i2);
    y_test_assert(world.entity_count() == 6);

    EntityPrefab empty;
    y_test_assert(empty.is_empty());
    y_test_assert(!empty.original_id().is_valid());
}



// ---------------------------------------- Inspection ----------------------------------------

y_test_func("EntityWorld inspect components") {
    EntityWorld world;

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    world.add_or_replace_component<Named>(a, core::String("before"), 2u);
    world.add_or_replace_component<Position>(a);
    world.add_or_replace_component<Velocity>(b);
    world.process_deferred_changes();

    {
        RecordingInspector inspector;
        world.inspect_components(a, &inspector);
        y_test_assert(inspector.types.size() == 2);
        y_test_assert(contains_type(inspector.types, type_index<Named>()));
        y_test_assert(contains_type(inspector.types, type_index<Position>()));
        for(usize i = 0; i != inspector.types.size(); ++i) {
            y_test_assert(inspector.has_inspects[i] == (inspector.types[i] == type_index<Named>()));
        }
        y_test_assert(inspector.visited.size() == 2);
        y_test_assert(std::find(inspector.visited.begin(), inspector.visited.end(), "name") != inspector.visited.end());
        y_test_assert(std::find(inspector.visited.begin(), inspector.visited.end(), "count") != inspector.visited.end());
    }

    // Inspection marks components as mutated
    y_test_assert(container_of<Named>(world)->mutated_ids().contains(a));
    y_test_assert(container_of<Position>(world)->mutated_ids().contains(a));
    world.process_deferred_changes();

    // Filtering by type
    {
        RecordingInspector inspector;
        inspector.new_name = "after";
        world.inspect_components(a, &inspector, type_index<Named>());
        y_test_assert(inspector.types.size() == 1);
        y_test_assert(inspector.types[0] == type_index<Named>());
        y_test_assert(world.component<Named>(a)->name == "after");
        y_test_assert(!container_of<Position>(world)->mutated_ids().contains(a));
    }
    world.process_deferred_changes();

    // Rejected inspection doesn't mutate
    {
        RecordingInspector inspector;
        inspector.accept = false;
        inspector.new_name = "rejected";
        world.inspect_components(a, &inspector);
        y_test_assert(inspector.types.size() == 2);
        y_test_assert(inspector.visited.is_empty());
        y_test_assert(world.component<Named>(a)->name == "after");
        y_test_assert(container_of<Named>(world)->mutated_ids().is_empty());
        y_test_assert(container_of<Position>(world)->mutated_ids().is_empty());
    }

    // Entity without the inspected type
    {
        RecordingInspector inspector;
        world.inspect_components(b, &inspector, type_index<Named>());
        y_test_assert(inspector.types.is_empty());
    }
}



// ---------------------------------------- Serialization ----------------------------------------

y_test_func("EntityWorld save and load") {
    EntityWorld world;

    core::Vector<EntityId> ids;
    for(u32 i = 0; i != 20; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id, float(i), float(i * 2));
        if(i % 2) {
            world.add_or_replace_component<Velocity>(id, float(i));
        }
        if(i % 5 == 0) {
            world.add_or_replace_component<Named>(id, core::String("named"), i);
            world.add_tag(id, "five");
        }
        ids << id;
    }
    world.add_or_replace_component<DeepDependent>(ids[3]);
    world.add_or_replace_component<Link>(ids[4], ids[7]);
    world.set_parent(ids[1], ids[0]);
    world.set_parent(ids[2], ids[0]);
    world.set_parent(ids[3], ids[2]);

    // Leave holes in the pool
    world.remove_entity(ids[10]);
    world.remove_entity(ids[11]);
    world.process_deferred_changes();

    EntityWorld loaded;
    save_and_load(world, loaded);

    y_test_assert(loaded.entity_count() == 18);
    y_test_assert(!loaded.exists(ids[10]));
    y_test_assert(!loaded.exists(ids[11]));

    for(u32 i = 0; i != 20; ++i) {
        const EntityId id = ids[i];
        if(i == 10 || i == 11) {
            continue;
        }
        y_test_assert(loaded.exists(id));
        y_test_assert(loaded.component<Position>(id));
        y_test_assert(loaded.component<Position>(id)->x == float(i));
        y_test_assert(loaded.component<Position>(id)->y == float(i * 2));
        y_test_assert(loaded.has_component<Velocity>(id) == (i % 2 == 1));
        y_test_assert(loaded.has_component<Named>(id) == (i % 5 == 0));
        y_test_assert(loaded.has_tag(id, "five") == (i % 5 == 0));
        if(i % 5 == 0) {
            y_test_assert(loaded.component<Named>(id)->name == "named");
            y_test_assert(loaded.component<Named>(id)->count == i);
        }
    }

    y_test_assert(loaded.has_component<DeepDependent>(ids[3]));
    y_test_assert(loaded.has_component<Dependent>(ids[3]));
    y_test_assert(loaded.is_component_required(ids[3], type_index<Dependent>()));
    y_test_assert(loaded.component<Link>(ids[4])->target == ids[7]);

    y_test_assert(loaded.parent(ids[1]) == ids[0]);
    y_test_assert(loaded.parent(ids[3]) == ids[2]);
    y_test_assert(loaded.is_parent(ids[3], ids[0]));
    y_test_assert(collect(loaded.direct_children(ids[0])).size() == 2);

    // 0, 5 and 15 (10 was removed)
    y_test_assert(loaded.tag_set("five")->size() == 3);

    // Groups are rebuilt from loaded data
    y_test_assert(loaded.create_group<Position>().size() == 18);
    y_test_assert((loaded.create_group<Position, Velocity>().size() == 9));
    std::array<std::string_view, 1> five = {"five"};
    y_test_assert(loaded.create_group<Named>(five).size() == 3);

    // Everything is considered changed after a load
    y_test_assert(loaded.create_group<Changed<Position>>().size() == 18);
    loaded.process_deferred_changes();
    y_test_assert(loaded.create_group<Changed<Position>>().is_empty());

    // Free list is preserved
    const EntityId fresh = loaded.create_entity();
    y_test_assert(fresh.index() == ids[10].index() || fresh.index() == ids[11].index());
    y_test_assert(!loaded.exists(ids[10]) || !loaded.exists(ids[11]));

    // Loaded world is fully functional
    loaded.add_or_replace_component<Velocity>(fresh);
    loaded.remove_entity(ids[0]);
    loaded.process_deferred_changes();
    y_test_assert(!loaded.has_parent(ids[1]));
    y_test_assert(loaded.entity_count() == 18);

    // Save the loaded world again
    EntityWorld reloaded;
    save_and_load(loaded, reloaded);
    y_test_assert(reloaded.entity_count() == 18);
    y_test_assert(reloaded.has_component<Velocity>(fresh));
    y_test_assert(reloaded.component<Position>(ids[19])->x == 19.0f);
}

y_test_func("EntityWorld save and load empty world") {
    EntityWorld world;
    EntityWorld loaded;
    save_and_load(world, loaded);
    y_test_assert(loaded.entity_count() == 0);

    const EntityId id = loaded.create_entity();
    loaded.add_or_replace_component<Position>(id);
    y_test_assert(loaded.create_group<Position>().size() == 1);
}



// ---------------------------------------- Systems ----------------------------------------

class TestSystem : public System {
    public:
        TestSystem() : System("TestSystem") {
        }

        void setup(SystemScheduler& sched) override {
            ++setup_count;

            sched.schedule(SystemSchedule::TickSequential, "seq", [this] {
                seq_thread = std::this_thread::get_id();
                seq_order = order++;
            });

            const SystemJobHandle a = sched.schedule(SystemSchedule::Tick, "a", [this] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                tick_order = order++;
                a_done = true;
            });
            y_debug_assert(a.is_valid());

            sched.schedule(SystemSchedule::Tick, "b", [this] {
                b_saw_a = a_done.load();
            }, a);

            sched.schedule(SystemSchedule::Update, "update", [this](const EntityWorld& w, SystemScheduler::FirstTime first) {
                update_order = order++;
                first_time = first.value;
                seen_entities = w.entity_count();
                ++update_count;
            });

            sched.schedule(SystemSchedule::PostUpdate, "post", [this](EntityGroup<Mutate<Position>, Velocity>&& group) {
                post_order = order++;
                for(auto&& [pos, vel] : group) {
                    pos.x += vel.dx;
                }
            });
        }

        void reset() override {
            ++reset_count;
        }

        std::atomic<u32> order = 0;
        u32 seq_order = u32(-1);
        u32 tick_order = u32(-1);
        u32 update_order = u32(-1);
        u32 post_order = u32(-1);

        std::atomic<bool> a_done = false;
        std::atomic<bool> b_saw_a = false;

        bool first_time = false;
        usize seen_entities = 0;
        u32 update_count = 0;

        std::thread::id seq_thread;

        u32 setup_count = 0;
        u32 reset_count = 0;
};

class OtherSystem : public System {
    public:
        OtherSystem(u32 v) : System("OtherSystem"), value(v) {
        }

        void setup(SystemScheduler& sched) override {
            sched.schedule(SystemSchedule::Update, "other", [this](EntityGroup<Velocity>&& group) {
                count = group.size();
            });
        }

        u32 value = 0;
        usize count = 0;
};

y_test_func("EntityWorld systems") {
    concurrent::JobSystem job_system(4);
    EntityWorld world;

    for(usize i = 0; i != 6; ++i) {
        const EntityId id = world.create_entity();
        world.add_or_replace_component<Position>(id);
        if(i % 2) {
            world.add_or_replace_component<Velocity>(id, 1.0f);
        }
    }
    world.process_deferred_changes();

    y_test_assert(!world.find_system<TestSystem>());

    TestSystem* system = world.add_system<TestSystem>();
    y_test_assert(system);
    y_test_assert(world.find_system<TestSystem>() == system);
    y_test_assert(static_cast<const EntityWorld&>(world).find_system<TestSystem>() == system);
    y_test_assert(world.find_system<System>() == system);
    y_test_assert(!world.find_system<OtherSystem>());
    y_test_assert(system->name() == "TestSystem");
    y_test_assert(&system->world() == &world);
    y_test_assert(system->setup_count == 1);

    OtherSystem* other = world.add_system<OtherSystem>(12u);
    y_test_assert(other->value == 12);
    y_test_assert(world.find_system<OtherSystem>() == other);

    const TickId before = world.tick_id();
    world.tick(job_system);
    y_test_assert(world.tick_id() > before);
    y_test_assert(world.tick_id() == before.next());

    y_test_assert(system->seq_thread == std::this_thread::get_id());
    y_test_assert(system->b_saw_a);
    y_test_assert(system->seq_order < system->tick_order);
    y_test_assert(system->tick_order < system->update_order);
    y_test_assert(system->update_order < system->post_order);
    y_test_assert(system->first_time);
    y_test_assert(system->seen_entities == 6);
    y_test_assert(system->update_count == 1);
    y_test_assert(other->count == 3);

    for(auto&& [id, pos] : world.component_set<Position>()) {
        y_test_assert(pos.x == (world.has_component<Velocity>(id) ? 1.0f : 0.0f));
    }
    y_test_assert(container_of<Position>(world)->mutated_ids().size() == 3);
    world.process_deferred_changes();

    system->order = 0;
    system->a_done = false;
    system->b_saw_a = false;
    world.tick(job_system);
    y_test_assert(!system->first_time);
    y_test_assert(system->b_saw_a);
    y_test_assert(system->update_count == 2);
    y_test_assert(system->seq_order < system->tick_order);
    y_test_assert(system->update_order < system->post_order);

    for(auto&& [id, pos] : world.component_set<Position>()) {
        y_test_assert(pos.x == (world.has_component<Velocity>(id) ? 2.0f : 0.0f));
    }
}

struct ConflictTracker {
    std::atomic<u32> order = 0;
    std::atomic<u32> writing = 0;
    std::atomic<u32> reading = 0;
    std::atomic<bool> overlapped = false;

    u32 enter(bool write) {
        if(write) {
            overlapped = overlapped || reading || writing++;
        } else {
            ++reading;
            overlapped = overlapped || writing;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        if(write) {
            --writing;
        } else {
            --reading;
        }
        return order++;
    }
};

template<bool First>
class ConflictingSystem : public System {
    public:
        ConflictingSystem(ConflictTracker* tracker) : System(First ? "ConflictingSystemA" : "ConflictingSystemB"), _tracker(tracker) {
        }

        void setup(SystemScheduler& sched) override {
            if constexpr(First) {
                sched.schedule(SystemSchedule::Update, "write", [this](EntityGroup<Mutate<Position>>&&) {
                    write_order = _tracker->enter(true);
                });
                sched.schedule(SystemSchedule::Update, "read", [this](EntityGroup<Position, Velocity>&&) {
                    read_order = _tracker->enter(false);
                });
            } else {
                sched.schedule(SystemSchedule::Update, "write", [this](EntityGroup<Velocity, Mutate<Position>>&&) {
                    write_order = _tracker->enter(true);
                });
            }
        }

        u32 write_order = u32(-1);
        u32 read_order = u32(-1);

    private:
        ConflictTracker* _tracker = nullptr;
};

y_test_func("EntityWorld systems conflicting accesses") {
    concurrent::JobSystem job_system(4);
    EntityWorld world;

    const EntityId id = world.create_entity();
    world.add_or_replace_component<Position>(id);
    world.add_or_replace_component<Velocity>(id, 1.0f);
    world.process_deferred_changes();

    ConflictTracker tracker;
    const auto* a = world.add_system<ConflictingSystem<true>>(&tracker);
    const auto* b = world.add_system<ConflictingSystem<false>>(&tracker);

    for(usize i = 0; i != 4; ++i) {
        tracker.order = 0;
        world.tick(job_system);
        world.process_deferred_changes();

        // Conflicting tasks run one after the other, in registration order
        y_test_assert(!tracker.overlapped);
        y_test_assert(a->write_order == 0);
        y_test_assert(a->read_order == 1);
        y_test_assert(b->write_order == 2);
    }
}

y_test_func("EntityWorld registered components") {
    EntityWorld world;

    const EntityId id = world.create_entity();
    world.add_or_replace_component<Registered>(id);

    RegisteringSystem* system = world.add_system<RegisteringSystem>();
    y_test_assert(system->registered.size() == 1);
    y_test_assert(system->registered[0] == type_index<Registered>());

    // Other systems are not affected
    TestSystem* test = world.add_system<TestSystem>();
    y_test_assert(test);
    y_test_assert(system->registered.size() == 1);
}

y_test_func("EntityWorld component trigger subscriptions") {
    concurrent::JobSystem job_system(2);
    EntityWorld world;
    world.add_system<TriggerSystem>();

    const auto tick = [&] {
        world.tick(job_system);
        world.process_deferred_changes();
    };

    const TriggerManager& triggers = world.triggers();

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    const EntityId c = world.create_entity();
    world.add_or_replace_component<TriggerListening>(a);
    world.add_or_replace_component<TriggerListening>(c);

    // Collected during the first tick, subscribed at the start of the next one
    tick();
    y_test_assert(!triggers.is_listened<TestTrigger>(a));
    tick();
    y_test_assert(triggers.is_listened<TestTrigger>(a));
    y_test_assert(triggers.is_listened<TestTrigger>(c));
    y_test_assert(!triggers.is_listened<TestTrigger>(b));
    y_test_assert(!triggers.is_listened<OtherTestTrigger>(a));

    // Subscribing twice to the same type only calls the handler once
    world.triggers().emit(a, TestTrigger{1});
    world.triggers().emit(b, TestTrigger{2});
    world.tick(job_system);
    y_test_assert((world.component<TriggerListening>(a)->received == core::Vector<u32>{1}));
    y_test_assert(world.component<TriggerListening>(c)->received.is_empty());

    // Receiving a trigger does not mark the component as changed
    y_test_assert(container_of<TriggerListening>(world)->mutated_ids().is_empty());
    world.process_deferred_changes();

    // Changing the component changes the subscriptions
    world.component_mut<TriggerListening>(a)->listen_test = false;
    world.component_mut<TriggerListening>(a)->listen_other = true;
    tick();
    tick();
    y_test_assert(!triggers.is_listened<TestTrigger>(a));
    y_test_assert(triggers.is_listened<OtherTestTrigger>(a));

    world.triggers().emit(a, TestTrigger{3});
    world.triggers().emit(a, OtherTestTrigger{4});
    tick();
    y_test_assert((world.component<TriggerListening>(a)->received == core::Vector<u32>{1, 104}));

    y_test_assert(triggers.is_listened(trigger_index<OtherTestTrigger>(), a));
    y_test_assert(!triggers.is_listened(trigger_index<TestTrigger>(), a));

    // Removing the component unsubscribes
    world.remove_component<TriggerListening>(a);
    tick();
    tick();
    y_test_assert(!triggers.is_listened<OtherTestTrigger>(a));

    // Deleting the entity unsubscribes
    world.remove_entity(c);
    tick();
    tick();
    y_test_assert(!triggers.is_listened<TestTrigger>(c));
}

std::unique_ptr<BlueprintNode> create_blueprint_node(std::string_view name) {
    core::Vector<std::unique_ptr<BlueprintNodeFactory>> factories;
    add_all_nodes(factories);
    for(const auto& factory : factories) {
        if(factory->name() == name) {
            return factory->create_node();
        }
    }
    return nullptr;
}

y_test_func("Blueprint shared nodes") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    {
        auto two = create_blueprint_node("Const float");
        *static_cast<float*>(two->param_ptr(0)) = 2.0f;
        const BlueprintNode* constant = blueprint.add_node(std::move(two));
        const BlueprintNode* neg = blueprint.add_node(create_blueprint_node("Negate float"));
        blueprint.add_link(constant, 0, neg, 0);

        const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* add = blueprint.add_node(create_blueprint_node("Add float"));
        const BlueprintNode* out_a = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        blueprint.add_link(on_a, 0, add, 0);
        blueprint.add_link(neg, 0, add, 1);
        blueprint.add_link(add, 0, out_a, 0);

        const BlueprintNode* on_b = blueprint.add_node(std::make_unique<TriggerBlueprintNode<OtherBlueprintTestTrigger>>("on b"));
        const BlueprintNode* mul = blueprint.add_node(create_blueprint_node("Multiply float"));
        const BlueprintNode* out_b = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(1));
        blueprint.add_link(on_b, 0, mul, 0);
        blueprint.add_link(neg, 0, mul, 1);
        blueprint.add_link(mul, 0, out_b, 0);
    }

    auto instance = blueprint.create_instance();
    y_test_assert(instance.is_ok());
    y_test_assert(instance.unwrap().entry_points().size() == 2);

    const BlueprintTestTrigger a{3.0f};
    y_test_assert(instance.unwrap().trigger(instance.unwrap().entry_points()[0], &a).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 1.0f);

    const OtherBlueprintTestTrigger b{4.0f};
    y_test_assert(instance.unwrap().trigger(instance.unwrap().entry_points()[1], &b).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[1] == -8.0f);
}

y_test_func("Blueprint serialization") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    {
        const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        blueprint.add_link(on_a, 0, select, 1);
        blueprint.add_link(select, 0, out, 0);
    }

    io2::Buffer buffer;
    {
        serde3::WritableArchive arc(buffer);
        y_test_assert(arc.serialize(blueprint).is_ok());
        buffer.reset();
    }

    // Generic types are not serialized and need to be resolved again
    Blueprint loaded;
    y_test_assert(serde3::ReadableArchive(buffer).deserialize(loaded).is_ok());
    y_test_assert(loaded.all_nodes().size() == 3);
    y_test_assert(loaded.all_nodes()[1]->generic_type() == blueprint_param_type<float>());

    auto instance = loaded.create_instance();
    y_test_assert(instance.is_ok());

    const BlueprintTestTrigger a{5.0f};
    y_test_assert(instance.unwrap().trigger(instance.unwrap().entry_points()[0], &a).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 5.0f);
}

y_test_func("Blueprint runtime errors") {
    Blueprint blueprint;
    const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
    const BlueprintNode* div = blueprint.add_node(create_blueprint_node("Divide float"));
    blueprint.add_link(on_a, 0, div, 1);

    auto instance = blueprint.create_instance();
    y_test_assert(instance.is_ok());

    const BlueprintTestTrigger zero{0.0f};
    const auto res = instance.unwrap().trigger(instance.unwrap().entry_points()[0], &zero);
    y_test_assert(res.is_error());
    y_test_assert(res.error().node == div);
    y_test_assert(res.error().error == "Division by zero");
}

y_test_func("Blueprint generic link replacement") {
    Blueprint blueprint;
    const BlueprintNode* f = blueprint.add_node(create_blueprint_node("Const float"));
    const BlueprintNode* v = blueprint.add_node(create_blueprint_node("Const Vec2"));
    const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));

    blueprint.add_link(f, 0, select, 1);
    y_test_assert(select->generic_type() == blueprint_param_type<float>());

    // Only generic link: replacing it with another type is allowed
    y_test_assert(blueprint.is_link_valid(v, 0, select, 1));
    blueprint.add_link(v, 0, select, 1);
    y_test_assert(select->generic_type() == blueprint_param_type<math::Vec2>());

    // Other generic pin is connected: the type is constrained
    y_test_assert(!blueprint.is_link_valid(f, 0, select, 2));
    blueprint.add_link(v, 0, select, 2);
    y_test_assert(!blueprint.is_link_valid(f, 0, select, 1));
}

y_test_func("EntityWorld blueprint component triggers") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    {
        const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* neg_a = blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* out_a = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        blueprint.add_link(on_a, 0, neg_a, 0);
        blueprint.add_link(neg_a, 0, out_a, 0);

        const BlueprintNode* on_b = blueprint.add_node(std::make_unique<TriggerBlueprintNode<OtherBlueprintTestTrigger>>("on b"));
        const BlueprintNode* neg_b = blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* out_b = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(1));
        blueprint.add_link(on_b, 0, neg_b, 0);
        blueprint.add_link(neg_b, 0, out_b, 0);
    }

    concurrent::JobSystem job_system(2);
    EntityWorld world;
    world.add_system<TriggerSystem>();

    const auto tick = [&] {
        world.tick(job_system);
        world.process_deferred_changes();
    };

    const EntityId id = world.create_entity();
    world.add_or_replace_component<BlueprintComponent>(id, make_asset<Blueprint>(std::move(blueprint)));

    tick();
    tick();
    y_test_assert(world.triggers().is_listened<BlueprintTestTrigger>(id));
    y_test_assert(world.triggers().is_listened<OtherBlueprintTestTrigger>(id));

    y_test_assert(world.component<BlueprintComponent>(id)->instance());

    // Only the nodes that depend on the fired trigger are evaluated
    world.triggers().emit(id, BlueprintTestTrigger{2.0f});
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[0] == -2.0f);
    y_test_assert(TestOutputBlueprintNode::outputs[1] == 0.0f);

    world.triggers().emit(id, OtherBlueprintTestTrigger{3.0f});
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[0] == -2.0f);
    y_test_assert(TestOutputBlueprintNode::outputs[1] == -3.0f);
}

y_test_func("EntityWorld load resets systems") {
    concurrent::JobSystem job_system(2);

    EntityWorld source;
    for(usize i = 0; i != 4; ++i) {
        const EntityId id = source.create_entity();
        source.add_or_replace_component<Position>(id);
        source.add_or_replace_component<Velocity>(id, 2.0f);
    }
    source.process_deferred_changes();

    EntityWorld world;
    TestSystem* system = world.add_system<TestSystem>();
    world.tick(job_system);
    y_test_assert(system->first_time);
    y_test_assert(system->seen_entities == 0);
    y_test_assert(system->reset_count == 0);

    save_and_load(source, world);
    y_test_assert(system->reset_count == 1);
    y_test_assert(system->setup_count == 2);
    y_test_assert(world.find_system<TestSystem>() == system);

    world.tick(job_system);
    y_test_assert(system->first_time);
    y_test_assert(system->seen_entities == 4);
    for(auto&& [pos] : world.create_group<Position>()) {
        y_test_assert(pos.x == 2.0f);
    }
}

y_test_func("EntityWorld tick without systems") {
    concurrent::JobSystem job_system(2);
    EntityWorld world;
    world.tick(job_system);
    world.tick(job_system);
    y_test_assert(world.tick_id() == TickId().next().next());
}



// ---------------------------------------- Blueprint graph ----------------------------------------

template<typename T>
const BlueprintNode* add_constant(Blueprint& blueprint, std::string_view name, T value) {
    auto node = create_blueprint_node(name);
    y_debug_assert(node);
    *static_cast<T*>(node->param_ptr(0)) = value;
    return blueprint.add_node(std::move(node));
}

usize node_index(const Blueprint& blueprint, const BlueprintNode* node) {
    const auto nodes = blueprint.all_nodes();
    for(usize i = 0; i != nodes.size(); ++i) {
        if(nodes[i].get() == node) {
            return i;
        }
    }
    return usize(-1);
}

const BlueprintNode* link_source(const Blueprint& blueprint, const BlueprintNode* dst, usize dst_pin) {
    const BlueprintLink* link = blueprint.find_link(dst, dst_pin);
    return link ? blueprint.all_nodes()[link->src_node].get() : nullptr;
}

bool links_are_ordered(const Blueprint& blueprint) {
    for(const BlueprintLink& link : blueprint.links()) {
        if(link.src_node >= link.dst_node) {
            return false;
        }
    }
    return true;
}

// Bypasses the Blueprint API to build broken graphs
core::Vector<BlueprintLink>& raw_links(Blueprint& blueprint) {
    return std::get<1>(y::reflect::list_members<Blueprint>()).get(blueprint);
}

// Runs the first entry point
core::Result<void, BlueprintError> run_blueprint(const Blueprint& blueprint, float value = 0.0f) {
    auto instance = blueprint.create_instance();
    if(instance.is_error()) {
        return core::Err(std::move(instance.error()));
    }
    const BlueprintTestTrigger trigger{value};
    return instance.unwrap().trigger(instance.unwrap().entry_points()[0], &trigger);
}

// Builds constants -> op -> If(1, -1) -> output, returns nothing on error
template<typename T>
std::optional<bool> eval_predicate(std::string_view op_name, std::string_view const_name, std::initializer_list<T> inputs) {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("trigger"));

    core::Vector<const BlueprintNode*> constants;
    for(const T& in : inputs) {
        constants << add_constant(blueprint, const_name, in);
    }
    const BlueprintNode* if_true = add_constant(blueprint, "Const float", 1.0f);
    const BlueprintNode* if_false = add_constant(blueprint, "Const float", -1.0f);
    const BlueprintNode* op = blueprint.add_node(create_blueprint_node(op_name));
    const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(3));

    for(usize i = 0; i != constants.size(); ++i) {
        blueprint.add_link(constants[i], 0, op, i);
    }
    blueprint.add_link(op, 0, select, 0);
    blueprint.add_link(if_true, 0, select, 1);
    blueprint.add_link(if_false, 0, select, 2);
    blueprint.add_link(select, 0, out, 0);

    if(run_blueprint(blueprint).is_error()) {
        return std::nullopt;
    }

    const float result = TestOutputBlueprintNode::outputs[3];
    if(result == 0.0f) {
        return std::nullopt;
    }
    return result > 0.0f;
}

template<typename T>
Blueprint make_trigger_output_blueprint(u32 index) {
    Blueprint blueprint;
    const BlueprintNode* on = blueprint.add_node(std::make_unique<TriggerBlueprintNode<T>>("on"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(index));
    blueprint.add_link(on, 0, out, 0);
    return blueprint;
}

y_test_func("Blueprint remove node") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
    const BlueprintNode* constant = add_constant(blueprint, "Const float", 5.0f);
    const BlueprintNode* add = blueprint.add_node(create_blueprint_node("Add float"));
    const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));

    blueprint.add_link(on_a, 0, add, 0);
    blueprint.add_link(constant, 0, add, 1);
    blueprint.add_link(add, 0, out, 0);
    blueprint.add_link(constant, 0, select, 1);
    y_test_assert(select->generic_type() == blueprint_param_type<float>());
    y_test_assert(blueprint.links().size() == 4);

    y_test_assert(run_blueprint(blueprint, 3.0f).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 8.0f);

    // Links touching the node are removed, the others are remapped
    blueprint.remove_node(constant);
    y_test_assert(blueprint.all_nodes().size() == 4);
    y_test_assert(blueprint.links().size() == 2);
    y_test_assert(node_index(blueprint, constant) == usize(-1));
    y_test_assert(!blueprint.find_link(add, 1));
    y_test_assert(!blueprint.find_link(select, 1));
    y_test_assert(link_source(blueprint, add, 0) == on_a);
    y_test_assert(link_source(blueprint, out, 0) == add);
    y_test_assert(links_are_ordered(blueprint));

    // The If node lost its type
    y_test_assert(!select->generic_type());
    {
        const auto res = blueprint.create_instance();
        y_test_assert(res.is_error());
        y_test_assert(res.error().node == select);
        y_test_assert(res.error().error == "Unresolved generic type");
    }

    blueprint.remove_node(select);
    y_test_assert(blueprint.all_nodes().size() == 3);
    y_test_assert(blueprint.links().size() == 2);
    y_test_assert(link_source(blueprint, out, 0) == add);

    y_test_assert(run_blueprint(blueprint, 3.0f).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 3.0f);
}

y_test_func("Blueprint remove and replace links") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
    const BlueprintNode* one = add_constant(blueprint, "Const float", 1.0f);
    const BlueprintNode* two = add_constant(blueprint, "Const float", 2.0f);
    const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(1));

    blueprint.add_link(one, 0, select, 1);
    blueprint.add_link(two, 0, select, 2);
    blueprint.add_link(select, 0, out, 0);
    y_test_assert(blueprint.links().size() == 3);

    // Linking into an already linked pin replaces the link
    y_test_assert(blueprint.is_link_valid(two, 0, select, 1));
    blueprint.add_link(two, 0, select, 1);
    y_test_assert(blueprint.links().size() == 3);
    y_test_assert(link_source(blueprint, select, 1) == two);
    y_test_assert(link_source(blueprint, select, 2) == two);

    y_test_assert(run_blueprint(blueprint).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[1] == 2.0f);

    // Unlinked generic input uses its (zero) default value
    blueprint.remove_link(select, 1);
    y_test_assert(blueprint.links().size() == 2);
    y_test_assert(!blueprint.find_link(select, 1));
    y_test_assert(select->generic_type() == blueprint_param_type<float>());

    TestOutputBlueprintNode::outputs[1] = 42.0f;
    y_test_assert(run_blueprint(blueprint).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[1] == 0.0f);

    // The output link alone is enough to resolve the type
    blueprint.remove_link(select, 2);
    y_test_assert(blueprint.links().size() == 1);
    y_test_assert(select->generic_type() == blueprint_param_type<float>());

    blueprint.remove_link(out, 0);
    y_test_assert(blueprint.links().is_empty());
    y_test_assert(!select->generic_type());

    // Removing a link that doesn't exist is fine
    blueprint.remove_link(out, 0);
    blueprint.remove_link(select, 0);
    y_test_assert(blueprint.links().is_empty());
}

y_test_func("Blueprint back links reorder nodes") {
    TestOutputBlueprintNode::outputs = {};

    {
        Blueprint blueprint;
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(2));
        const BlueprintNode* add = blueprint.add_node(create_blueprint_node("Add float"));
        const BlueprintNode* neg = blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* constant = add_constant(blueprint, "Const float", 10.0f);
        const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));

        blueprint.add_link(add, 0, out, 0);
        y_test_assert(links_are_ordered(blueprint));
        blueprint.add_link(neg, 0, add, 0);
        y_test_assert(links_are_ordered(blueprint));
        blueprint.add_link(constant, 0, add, 1);
        y_test_assert(links_are_ordered(blueprint));
        blueprint.add_link(on_a, 0, neg, 0);
        y_test_assert(links_are_ordered(blueprint));

        // Nodes are reordered but not lost
        y_test_assert(blueprint.all_nodes().size() == 5);
        for(const BlueprintNode* node : {out, add, neg, constant, on_a}) {
            y_test_assert(node_index(blueprint, node) != usize(-1));
        }
        y_test_assert(link_source(blueprint, out, 0) == add);
        y_test_assert(link_source(blueprint, add, 0) == neg);
        y_test_assert(link_source(blueprint, add, 1) == constant);
        y_test_assert(link_source(blueprint, neg, 0) == on_a);

        y_test_assert(blueprint.validate().is_ok());
        y_test_assert(run_blueprint(blueprint, 3.0f).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[2] == 7.0f);

        // Would create a cycle
        y_test_assert(!blueprint.is_link_valid(add, 0, neg, 0));
        y_test_assert(!blueprint.is_link_valid(neg, 0, on_a, 0));
    }

    {
        // Unrelated nodes in between the source and destination
        Blueprint blueprint;
        const BlueprintNode* neg = blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* constant = add_constant(blueprint, "Const float", 4.0f);
        const BlueprintNode* add = blueprint.add_node(create_blueprint_node("Add float"));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(3));
        const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));

        blueprint.add_link(neg, 0, add, 0);
        blueprint.add_link(constant, 0, add, 1);
        blueprint.add_link(add, 0, out, 0);
        blueprint.add_link(on_a, 0, neg, 0);
        y_test_assert(links_are_ordered(blueprint));
        y_test_assert(link_source(blueprint, add, 0) == neg);
        y_test_assert(link_source(blueprint, add, 1) == constant);
        y_test_assert(link_source(blueprint, out, 0) == add);
        y_test_assert(link_source(blueprint, neg, 0) == on_a);

        y_test_assert(run_blueprint(blueprint, 3.0f).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[3] == 1.0f);
    }
}

y_test_func("Blueprint add blueprint") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    {
        const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* neg = blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        blueprint.add_link(on_a, 0, neg, 0);
        blueprint.add_link(neg, 0, out, 0);
    }

    const BlueprintNode* select = nullptr;
    const BlueprintNode* out_b = nullptr;
    {
        Blueprint other;
        const BlueprintNode* constant = add_constant(other, "Const float", 7.0f);
        select = other.add_node(create_blueprint_node("If"));
        out_b = other.add_node(std::make_unique<TestOutputBlueprintNode>(1));
        other.add_link(constant, 0, select, 1);
        other.add_link(select, 0, out_b, 0);

        blueprint.add_blueprint(std::move(other));
    }

    y_test_assert(blueprint.all_nodes().size() == 6);
    y_test_assert(blueprint.links().size() == 4);
    y_test_assert(links_are_ordered(blueprint));
    y_test_assert(node_index(blueprint, select) == 4);
    y_test_assert(link_source(blueprint, out_b, 0) == select);
    y_test_assert(select->generic_type() == blueprint_param_type<float>());
    y_test_assert(blueprint.validate().is_ok());

    y_test_assert(run_blueprint(blueprint, 2.0f).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[0] == -2.0f);
    y_test_assert(TestOutputBlueprintNode::outputs[1] == 7.0f);

    // Adding an empty blueprint does nothing
    blueprint.add_blueprint(Blueprint());
    y_test_assert(blueprint.all_nodes().size() == 6);
    y_test_assert(blueprint.links().size() == 4);
}

y_test_func("Blueprint invalid links") {
    Blueprint blueprint;
    const BlueprintNode* flag = add_constant(blueprint, "Const bool", true);
    const BlueprintNode* a = blueprint.add_node(create_blueprint_node("Negate float"));
    const BlueprintNode* b = blueprint.add_node(create_blueprint_node("Negate float"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));

    Blueprint other;
    const BlueprintNode* foreign = other.add_node(create_blueprint_node("Negate float"));

    y_test_assert(blueprint.is_link_valid(a, 0, b, 0));
    y_test_assert(blueprint.is_link_valid(b, 0, a, 0));

    y_test_assert(!blueprint.is_link_valid(a, 0, a, 0));
    y_test_assert(!blueprint.is_link_valid(nullptr, 0, b, 0));
    y_test_assert(!blueprint.is_link_valid(a, 0, nullptr, 0));
    y_test_assert(!blueprint.is_link_valid(a, 1, b, 0));
    y_test_assert(!blueprint.is_link_valid(a, 0, b, 1));
    y_test_assert(!blueprint.is_link_valid(out, 0, a, 0));
    y_test_assert(!blueprint.is_link_valid(foreign, 0, b, 0));
    y_test_assert(!blueprint.is_link_valid(a, 0, foreign, 0));
    y_test_assert(!blueprint.is_link_valid(flag, 0, b, 0));

    blueprint.add_link(a, 0, b, 0);
    y_test_assert(!blueprint.is_link_valid(b, 0, a, 0));
    y_test_assert(blueprint.is_link_valid(b, 0, out, 0));
}

y_test_func("Blueprint downstream nodes") {
    Blueprint blueprint;
    const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
    const BlueprintNode* constant = add_constant(blueprint, "Const float", 1.0f);
    const BlueprintNode* n1 = blueprint.add_node(create_blueprint_node("Negate float"));
    const BlueprintNode* n2 = blueprint.add_node(create_blueprint_node("Negate float"));
    const BlueprintNode* add = blueprint.add_node(create_blueprint_node("Add float"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));

    blueprint.add_link(on_a, 0, n1, 0);
    blueprint.add_link(on_a, 0, n2, 0);
    blueprint.add_link(n1, 0, add, 0);
    blueprint.add_link(n2, 0, add, 1);
    blueprint.add_link(add, 0, out, 0);

    const auto is_downstream = [&](const core::FixedArray<bool>& downstream, const BlueprintNode* node) {
        return downstream[node_index(blueprint, node)];
    };

    {
        const auto downstream = blueprint.downstream_nodes(on_a);
        y_test_assert(downstream.size() == 6);
        for(const BlueprintNode* node : {on_a, n1, n2, add, out}) {
            y_test_assert(is_downstream(downstream, node));
        }
        y_test_assert(!is_downstream(downstream, constant));
    }

    {
        const auto downstream = blueprint.downstream_nodes(n1);
        y_test_assert(is_downstream(downstream, n1));
        y_test_assert(is_downstream(downstream, add));
        y_test_assert(is_downstream(downstream, out));
        y_test_assert(!is_downstream(downstream, n2));
        y_test_assert(!is_downstream(downstream, on_a));
        y_test_assert(!is_downstream(downstream, constant));
    }

    {
        const auto downstream = blueprint.downstream_nodes(constant);
        for(const BlueprintNode* node : {on_a, n1, n2, add, out}) {
            y_test_assert(!is_downstream(downstream, node));
        }
        y_test_assert(is_downstream(downstream, constant));
    }
}

y_test_func("Blueprint validation errors") {
    const auto check_error = [](const Blueprint& blueprint, const BlueprintNode* node, std::string_view message) {
        const auto res = blueprint.validate();
        if(!res.is_error() || res.error().node != node || res.error().error != message) {
            return false;
        }
        const auto instance = blueprint.create_instance();
        return instance.is_error() && instance.error().node == node && instance.error().error == message;
    };

    {
        Blueprint blueprint;
        const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));
        y_test_assert(check_error(blueprint, select, "Unresolved generic type"));
    }

    {
        Blueprint blueprint;
        blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* b = blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{99, 0, 1, 0};
        y_test_assert(check_error(blueprint, b, "Link references an invalid node or pin"));
    }

    {
        Blueprint blueprint;
        blueprint.add_node(create_blueprint_node("Negate float"));
        blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{0, 0, 99, 0};
        y_test_assert(check_error(blueprint, nullptr, "Link references an invalid node or pin"));
    }

    {
        Blueprint blueprint;
        blueprint.add_node(create_blueprint_node("Negate float"));
        const BlueprintNode* b = blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{0, 5, 1, 0};
        y_test_assert(check_error(blueprint, b, "Link references an invalid node or pin"));
    }

    {
        Blueprint blueprint;
        const BlueprintNode* a = blueprint.add_node(create_blueprint_node("Negate float"));
        blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{1, 0, 0, 0};
        y_test_assert(check_error(blueprint, a, "Link breaks execution order"));
    }

    {
        Blueprint blueprint;
        const BlueprintNode* a = blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{0, 0, 0, 0};
        y_test_assert(check_error(blueprint, a, "Link breaks execution order"));
    }

    {
        Blueprint blueprint;
        add_constant(blueprint, "Const bool", true);
        const BlueprintNode* neg = blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{0, 0, 1, 0};
        y_test_assert(check_error(blueprint, neg, "Link connects incompatible types"));
    }

    {
        Blueprint blueprint;
        blueprint.add_node(create_blueprint_node("Negate float"));
        blueprint.add_node(create_blueprint_node("Negate float"));
        raw_links(blueprint) << BlueprintLink{0, 0, 1, 0};
        y_test_assert(blueprint.validate().is_ok());
        y_test_assert(blueprint.create_instance().is_ok());
    }
}



// ---------------------------------------- Blueprint nodes ----------------------------------------

y_test_func("Blueprint bool nodes") {
    for(const bool a : {false, true}) {
        y_test_assert(eval_predicate<bool>("Not", "Const bool", {a}) == !a);
        for(const bool b : {false, true}) {
            y_test_assert(eval_predicate<bool>("And", "Const bool", {a, b}) == (a && b));
            y_test_assert(eval_predicate<bool>("Or", "Const bool", {a, b}) == (a || b));
            y_test_assert(eval_predicate<bool>("Xor", "Const bool", {a, b}) == (a != b));
        }
    }
}

y_test_func("Blueprint comparison nodes") {
    const std::array<std::pair<float, float>, 3> pairs = {{{1.0f, 2.0f}, {2.0f, 2.0f}, {3.0f, 2.0f}}};
    for(const auto& [a, b] : pairs) {
        y_test_assert(eval_predicate<float>("Equal float", "Const float", {a, b}) == (a == b));
        y_test_assert(eval_predicate<float>("Not equal float", "Const float", {a, b}) == (a != b));
        y_test_assert(eval_predicate<float>("Less float", "Const float", {a, b}) == (a < b));
        y_test_assert(eval_predicate<float>("Greater float", "Const float", {a, b}) == (a > b));
        y_test_assert(eval_predicate<float>("Less or equal float", "Const float", {a, b}) == (a <= b));
        y_test_assert(eval_predicate<float>("Greater or equal float", "Const float", {a, b}) == (a >= b));
    }

    y_test_assert(eval_predicate<math::Vec3>("Equal Vec3", "Const Vec3", {math::Vec3(1.0f, 2.0f, 3.0f), math::Vec3(1.0f, 2.0f, 3.0f)}) == true);
    y_test_assert(eval_predicate<math::Vec3>("Equal Vec3", "Const Vec3", {math::Vec3(1.0f, 2.0f, 3.0f), math::Vec3(1.0f, 2.0f, 4.0f)}) == false);
    y_test_assert(eval_predicate<math::Vec3>("Not equal Vec3", "Const Vec3", {math::Vec3(1.0f, 2.0f, 3.0f), math::Vec3(1.0f, 2.0f, 4.0f)}) == true);
}

y_test_func("Blueprint vector nodes") {
    // Create -> Decompose
    {
        TestOutputBlueprintNode::outputs = {};
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* x = add_constant(blueprint, "Const float", 1.0f);
        const BlueprintNode* y = add_constant(blueprint, "Const float", 2.0f);
        const BlueprintNode* z = add_constant(blueprint, "Const float", 3.0f);
        const BlueprintNode* create = blueprint.add_node(create_blueprint_node("Create Vec3"));
        const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node("Decompose Vec3"));
        blueprint.add_link(x, 0, create, 0);
        blueprint.add_link(y, 0, create, 1);
        blueprint.add_link(z, 0, create, 2);
        blueprint.add_link(create, 0, decompose, 0);
        for(u32 i = 0; i != 3; ++i) {
            const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(i));
            blueprint.add_link(decompose, i, out, 0);
        }

        y_test_assert(run_blueprint(blueprint).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == 1.0f);
        y_test_assert(TestOutputBlueprintNode::outputs[1] == 2.0f);
        y_test_assert(TestOutputBlueprintNode::outputs[2] == 3.0f);
    }

    // Vector -> float ops
    const auto eval_scalar = [](std::string_view op_name, std::string_view const_name, auto a, auto b) -> std::optional<float> {
        TestOutputBlueprintNode::outputs = {};
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* op = nullptr;
        const BlueprintNode* ca = add_constant(blueprint, const_name, a);
        const BlueprintNode* cb = add_constant(blueprint, const_name, b);
        op = blueprint.add_node(create_blueprint_node(op_name));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        blueprint.add_link(ca, 0, op, 0);
        if(op->input_pins().size() > 1) {
            blueprint.add_link(cb, 0, op, 1);
        }
        blueprint.add_link(op, 0, out, 0);
        if(run_blueprint(blueprint).is_error()) {
            return std::nullopt;
        }
        return TestOutputBlueprintNode::outputs[0];
    };

    y_test_assert(eval_scalar("Dot Vec3", "Const Vec3", math::Vec3(1.0f, 2.0f, 3.0f), math::Vec3(4.0f, 5.0f, 6.0f)) == 32.0f);
    y_test_assert(eval_scalar("Length Vec2", "Const Vec2", math::Vec2(3.0f, 4.0f), math::Vec2()) == 5.0f);
    y_test_assert(eval_scalar("Divide float", "Const float", 6.0f, 3.0f) == 2.0f);
    y_test_assert(eval_scalar("Multiply float", "Const float", 6.0f, 3.0f) == 18.0f);
    y_test_assert(!eval_scalar("Divide float", "Const float", 6.0f, 0.0f));

    // Vector -> vector ops
    const auto eval_vec = [](std::string_view op_name, std::string_view decompose_name, std::string_view const_name, auto a, auto b) -> std::optional<std::array<float, 4>> {
        TestOutputBlueprintNode::outputs = {};
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* ca = add_constant(blueprint, const_name, a);
        const BlueprintNode* cb = add_constant(blueprint, const_name, b);
        const BlueprintNode* op = blueprint.add_node(create_blueprint_node(op_name));
        const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node(decompose_name));
        blueprint.add_link(ca, 0, op, 0);
        if(op->input_pins().size() > 1) {
            blueprint.add_link(cb, 0, op, 1);
        }
        blueprint.add_link(op, 0, decompose, 0);
        for(u32 i = 0; i != decompose->output_pins().size(); ++i) {
            const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(i));
            blueprint.add_link(decompose, i, out, 0);
        }
        if(run_blueprint(blueprint).is_error()) {
            return std::nullopt;
        }
        return TestOutputBlueprintNode::outputs;
    };

    {
        const auto res = eval_vec("Add Vec2", "Decompose Vec2", "Const Vec2", math::Vec2(1.0f, 2.0f), math::Vec2(3.0f, 4.0f));
        y_test_assert(res && (*res)[0] == 4.0f && (*res)[1] == 6.0f);
    }
    {
        const auto res = eval_vec("Cross Vec3", "Decompose Vec3", "Const Vec3", math::Vec3(1.0f, 0.0f, 0.0f), math::Vec3(0.0f, 1.0f, 0.0f));
        y_test_assert(res && (*res)[0] == 0.0f && (*res)[1] == 0.0f && (*res)[2] == 1.0f);
    }
    {
        const auto res = eval_vec("Negate Vec4", "Decompose Vec4", "Const Vec4", math::Vec4(1.0f, -2.0f, 3.0f, -4.0f), math::Vec4());
        y_test_assert(res && (*res)[0] == -1.0f && (*res)[1] == 2.0f && (*res)[2] == -3.0f && (*res)[3] == 4.0f);
    }
    {
        const auto res = eval_vec("Abs Vec2", "Decompose Vec2", "Const Vec2", math::Vec2(-1.0f, 2.0f), math::Vec2());
        y_test_assert(res && (*res)[0] == 1.0f && (*res)[1] == 2.0f);
    }
    {
        const auto res = eval_vec("Saturate Vec2", "Decompose Vec2", "Const Vec2", math::Vec2(-1.0f, 2.0f), math::Vec2());
        y_test_assert(res && (*res)[0] == 0.0f && (*res)[1] == 1.0f);
    }
    {
        const auto res = eval_vec("Normalize Vec2", "Decompose Vec2", "Const Vec2", math::Vec2(3.0f, 4.0f), math::Vec2());
        y_test_assert(res && std::abs((*res)[0] - 0.6f) < 0.0001f && std::abs((*res)[1] - 0.8f) < 0.0001f);
    }
    {
        const auto res = eval_vec("Divide Vec2", "Decompose Vec2", "Const Vec2", math::Vec2(4.0f, 9.0f), math::Vec2(2.0f, 3.0f));
        y_test_assert(res && (*res)[0] == 2.0f && (*res)[1] == 3.0f);
    }

    // Any zero component is a division by zero
    {
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* a = add_constant(blueprint, "Const Vec3", math::Vec3(1.0f, 1.0f, 1.0f));
        const BlueprintNode* b = add_constant(blueprint, "Const Vec3", math::Vec3(1.0f, 0.0f, 1.0f));
        const BlueprintNode* div = blueprint.add_node(create_blueprint_node("Divide Vec3"));
        blueprint.add_link(a, 0, div, 0);
        blueprint.add_link(b, 0, div, 1);

        const auto res = run_blueprint(blueprint);
        y_test_assert(res.is_error());
        y_test_assert(res.error().node == div);
        y_test_assert(res.error().error == "Division by zero");
    }

    // If on vectors with default condition
    {
        TestOutputBlueprintNode::outputs = {};
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* a = add_constant(blueprint, "Const Vec2", math::Vec2(1.0f, 2.0f));
        const BlueprintNode* b = add_constant(blueprint, "Const Vec2", math::Vec2(3.0f, 4.0f));
        const BlueprintNode* select = blueprint.add_node(create_blueprint_node("If"));
        const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node("Decompose Vec2"));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        blueprint.add_link(a, 0, select, 1);
        blueprint.add_link(b, 0, select, 2);
        blueprint.add_link(select, 0, decompose, 0);
        blueprint.add_link(decompose, 1, out, 0);

        y_test_assert(run_blueprint(blueprint).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == 2.0f);
    }
}

y_test_func("Blueprint node factories") {
    core::Vector<std::unique_ptr<BlueprintNodeFactory>> factories;
    add_all_nodes(factories);
    y_test_assert(!factories.is_empty());

    for(usize i = 0; i != factories.size(); ++i) {
        for(usize j = i + 1; j != factories.size(); ++j) {
            y_test_assert(factories[i]->name() != factories[j]->name());
        }
    }

    for(const auto& factory : factories) {
        std::unique_ptr<BlueprintNode> node = factory->create_node();
        y_test_assert(node);
        y_test_assert(node->name() == factory->name());
        y_test_assert(!node->node_type_name().empty());
        y_test_assert(!node->generic_type());

        Blueprint blueprint;
        const BlueprintNode* added = blueprint.add_node(std::move(node));

        // Every node compiles with its default inputs, unless it needs a type
        y_test_assert(blueprint.create_instance().is_ok() != added->has_generic_pin());

        io2::Buffer buffer;
        {
            serde3::WritableArchive arc(buffer);
            y_test_assert(arc.serialize(blueprint).is_ok());
        }
        buffer.reset();

        Blueprint loaded;
        y_test_assert(serde3::ReadableArchive(buffer).deserialize(loaded).is_ok());
        y_test_assert(loaded.all_nodes().size() == 1);

        const BlueprintNode* loaded_node = loaded.all_nodes()[0].get();
        y_test_assert(loaded_node->name() == factory->name());
        y_test_assert(loaded_node->node_type_name() == added->node_type_name());
        y_test_assert(loaded_node->input_pins().size() == added->input_pins().size());
        y_test_assert(loaded_node->output_pins().size() == added->output_pins().size());
        y_test_assert(loaded_node->is_entry_point() == added->is_entry_point());
    }
}

y_test_func("Blueprint If default values") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));

    std::unique_ptr<BlueprintNode> select_node = create_blueprint_node("If");
    BlueprintNode* select = select_node.get();
    blueprint.add_node(std::move(select_node));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));

    // No type, no storage for the values
    y_test_assert(!select->default_input(1));
    y_test_assert(!select->default_input(2));

    blueprint.add_link(select, 0, out, 0);
    y_test_assert(select->generic_type() == blueprint_param_type<float>());
    *static_cast<bool*>(select->default_input(0)) = false;
    *static_cast<float*>(select->default_input(1)) = 3.0f;
    *static_cast<float*>(select->default_input(2)) = 4.0f;

    y_test_assert(run_blueprint(blueprint).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 4.0f);

    // Default values survive serialization
    {
        io2::Buffer buffer;
        {
            serde3::WritableArchive arc(buffer);
            y_test_assert(arc.serialize(blueprint).is_ok());
        }
        buffer.reset();

        Blueprint loaded;
        y_test_assert(serde3::ReadableArchive(buffer).deserialize(loaded).is_ok());

        BlueprintNode* loaded_select = loaded.all_nodes()[1].get();
        y_test_assert(loaded_select->generic_type() == blueprint_param_type<float>());
        y_test_assert(*static_cast<bool*>(loaded_select->default_input(0)) == false);
        y_test_assert(*static_cast<float*>(loaded_select->default_input(1)) == 3.0f);
        y_test_assert(*static_cast<float*>(loaded_select->default_input(2)) == 4.0f);

        TestOutputBlueprintNode::outputs = {};
        y_test_assert(run_blueprint(loaded).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == 4.0f);
    }

    // Losing the type and getting it back keeps the values
    blueprint.remove_link(out, 0);
    y_test_assert(!select->generic_type());
    blueprint.add_link(select, 0, out, 0);
    y_test_assert(*static_cast<float*>(select->default_input(1)) == 3.0f);
    y_test_assert(*static_cast<float*>(select->default_input(2)) == 4.0f);

    // Changing the type resets them
    blueprint.remove_link(out, 0);
    const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node("Decompose Vec2"));
    blueprint.add_link(select, 0, decompose, 0);
    y_test_assert(select->generic_type() == blueprint_param_type<math::Vec2>());
    y_test_assert(*static_cast<math::Vec2*>(select->default_input(1)) == math::Vec2());
    y_test_assert(*static_cast<math::Vec2*>(select->default_input(2)) == math::Vec2());
}

y_test_func("Blueprint more vector nodes") {
    y_test_assert(eval_predicate<math::Vec2>("Equal Vec2", "Const Vec2", {math::Vec2(1.0f, 2.0f), math::Vec2(1.0f, 2.0f)}) == true);
    y_test_assert(eval_predicate<math::Vec2>("Equal Vec2", "Const Vec2", {math::Vec2(1.0f, 2.0f), math::Vec2(2.0f, 1.0f)}) == false);
    y_test_assert(eval_predicate<math::Vec2>("Not equal Vec2", "Const Vec2", {math::Vec2(1.0f, 2.0f), math::Vec2(1.0f, 2.0f)}) == false);

    // Normalizing a zero vector gives a zero vector
    {
        TestOutputBlueprintNode::outputs = {1.0f, 1.0f, 1.0f, 1.0f};
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* zero = add_constant(blueprint, "Const Vec3", math::Vec3());
        const BlueprintNode* normalize = blueprint.add_node(create_blueprint_node("Normalize Vec3"));
        const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node("Decompose Vec3"));
        blueprint.add_link(zero, 0, normalize, 0);
        blueprint.add_link(normalize, 0, decompose, 0);
        for(u32 i = 0; i != 3; ++i) {
            const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(i));
            blueprint.add_link(decompose, i, out, 0);
        }

        y_test_assert(run_blueprint(blueprint).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == 0.0f);
        y_test_assert(TestOutputBlueprintNode::outputs[1] == 0.0f);
        y_test_assert(TestOutputBlueprintNode::outputs[2] == 0.0f);
    }

    // Saturate and Abs on mixed sign input
    {
        TestOutputBlueprintNode::outputs = {};
        Blueprint blueprint;
        blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* in = add_constant(blueprint, "Const Vec4", math::Vec4(-2.0f, -0.5f, 0.5f, 2.0f));
        const BlueprintNode* abs = blueprint.add_node(create_blueprint_node("Abs Vec4"));
        const BlueprintNode* saturate = blueprint.add_node(create_blueprint_node("Saturate Vec4"));
        const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node("Decompose Vec4"));
        blueprint.add_link(in, 0, abs, 0);
        blueprint.add_link(abs, 0, saturate, 0);
        blueprint.add_link(saturate, 0, decompose, 0);
        for(u32 i = 0; i != 4; ++i) {
            const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(i));
            blueprint.add_link(decompose, i, out, 0);
        }

        y_test_assert(run_blueprint(blueprint).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == 1.0f);
        y_test_assert(TestOutputBlueprintNode::outputs[1] == 0.5f);
        y_test_assert(TestOutputBlueprintNode::outputs[2] == 0.5f);
        y_test_assert(TestOutputBlueprintNode::outputs[3] == 1.0f);
    }
}



// ---------------------------------------- Blueprint instances ----------------------------------------

y_test_func("Blueprint instance repeated triggers") {
    TestOutputBlueprintNode::outputs = {};

    Blueprint blueprint;
    const BlueprintNode* on_a = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
    const BlueprintNode* neg = blueprint.add_node(create_blueprint_node("Negate float"));
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
    blueprint.add_link(on_a, 0, neg, 0);
    blueprint.add_link(neg, 0, out, 0);

    auto first = blueprint.create_instance();
    y_test_assert(first.is_ok());
    BlueprintInstance& instance = first.unwrap();

    y_test_assert(instance.entry_points().size() == 1);
    const BlueprintInstance::EntryPoint& entry = instance.entry_points()[0];
    y_test_assert(entry.node_index == node_index(blueprint, on_a));
    y_test_assert(entry.trigger_type == trigger_index<BlueprintTestTrigger>());
    y_test_assert(entry.payload_size == sizeof(BlueprintTestTrigger));

    for(const float value : {1.0f, 2.0f, -5.0f}) {
        const BlueprintTestTrigger trigger{value};
        y_test_assert(instance.trigger(entry, &trigger).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == -value);
    }

    // Instances don't share state
    auto second = blueprint.create_instance();
    y_test_assert(second.is_ok());
    {
        const BlueprintTestTrigger trigger{10.0f};
        y_test_assert(second.unwrap().trigger(second.unwrap().entry_points()[0], &trigger).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == -10.0f);
    }
    {
        const BlueprintTestTrigger trigger{1.0f};
        y_test_assert(instance.trigger(entry, &trigger).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[0] == -1.0f);
    }

    // Runtime errors don't break the instance
    {
        Blueprint div_blueprint;
        const BlueprintNode* on = div_blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* one = add_constant(div_blueprint, "Const float", 1.0f);
        const BlueprintNode* div = div_blueprint.add_node(create_blueprint_node("Divide float"));
        const BlueprintNode* div_out = div_blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(1));
        div_blueprint.add_link(one, 0, div, 0);
        div_blueprint.add_link(on, 0, div, 1);
        div_blueprint.add_link(div, 0, div_out, 0);

        auto div_instance = div_blueprint.create_instance();
        y_test_assert(div_instance.is_ok());
        const auto& div_entry = div_instance.unwrap().entry_points()[0];

        const BlueprintTestTrigger zero{0.0f};
        y_test_assert(div_instance.unwrap().trigger(div_entry, &zero).is_error());

        const BlueprintTestTrigger four{4.0f};
        y_test_assert(div_instance.unwrap().trigger(div_entry, &four).is_ok());
        y_test_assert(TestOutputBlueprintNode::outputs[1] == 0.25f);
    }
}

y_test_func("Blueprint unconnected nodes run for every entry point") {
    Blueprint blueprint;
    blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
    blueprint.add_node(std::make_unique<TriggerBlueprintNode<OtherBlueprintTestTrigger>>("on b"));
    const BlueprintNode* constant = add_constant(blueprint, "Const float", 5.0f);
    const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(2));
    blueprint.add_link(constant, 0, out, 0);

    auto instance = blueprint.create_instance();
    y_test_assert(instance.is_ok());
    y_test_assert(instance.unwrap().entry_points().size() == 2);

    TestOutputBlueprintNode::outputs = {};
    const BlueprintTestTrigger a{1.0f};
    y_test_assert(instance.unwrap().trigger(instance.unwrap().entry_points()[0], &a).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[2] == 5.0f);

    TestOutputBlueprintNode::outputs = {};
    const OtherBlueprintTestTrigger b{1.0f};
    y_test_assert(instance.unwrap().trigger(instance.unwrap().entry_points()[1], &b).is_ok());
    y_test_assert(TestOutputBlueprintNode::outputs[2] == 5.0f);
}



// ---------------------------------------- Triggers ----------------------------------------

y_test_func("TriggerManager subscriptions") {
    EntityWorld world;
    TriggerManager& triggers = world.triggers();

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    world.process_deferred_changes();

    core::Vector<EntityId> ids;
    core::Vector<u32> values;
    TriggerCallback<TestTrigger> handler([&](EntityWorld&, EntityId id, const TestTrigger& t) {
        ids << id;
        values << t.value;
    });

    bool built = false;
    const auto make = [&](u32 value) {
        return [&built, value] {
            built = true;
            return TestTrigger{value};
        };
    };

    // Nothing listens: emitting does nothing and doesn't build the payload
    triggers.emit<TestTrigger>(a, make(0));
    triggers.emit(a, TestTrigger{1});
    triggers.dispatch(world);
    y_test_assert(!built);
    y_test_assert(values.is_empty());
    y_test_assert(!triggers.is_listened<TestTrigger>(a));

    triggers.subscribe<TestTrigger>(a, &handler);
    y_test_assert(triggers.is_listened<TestTrigger>(a));
    y_test_assert(!triggers.is_listened<TestTrigger>(b));
    y_test_assert(!triggers.is_listened<OtherTestTrigger>(a));

    triggers.emit<TestTrigger>(b, make(10));
    y_test_assert(!built);
    triggers.emit<TestTrigger>(a, make(2));
    y_test_assert(built);
    triggers.emit(b, TestTrigger{3});
    triggers.emit(a, TestTrigger{4});

    // Nothing is delivered before dispatch
    y_test_assert(values.is_empty());
    triggers.dispatch(world);
    y_test_assert((values == core::Vector<u32>{2, 4}));
    y_test_assert((ids == core::Vector<EntityId>{a, a}));

    // Events are only delivered once
    triggers.dispatch(world);
    y_test_assert(values.size() == 2);

    // Global handlers receive everything
    core::Vector<u32> global_values;
    TriggerCallback<TestTrigger> global([&](EntityWorld&, EntityId, const TestTrigger& t) {
        global_values << t.value;
    });
    triggers.subscribe<TestTrigger>(EntityId(), &global);
    y_test_assert(triggers.is_listened<TestTrigger>(b));

    triggers.emit(b, TestTrigger{5});
    triggers.emit(a, TestTrigger{6});
    triggers.dispatch(world);
    y_test_assert((global_values == core::Vector<u32>{5, 6}));
    y_test_assert((values == core::Vector<u32>{2, 4, 6}));

    triggers.unsubscribe<TestTrigger>(EntityId(), &global);
    y_test_assert(!triggers.is_listened<TestTrigger>(b));
    y_test_assert(triggers.is_listened<TestTrigger>(a));

    // Events targeting deleted entities are dropped
    triggers.emit(a, TestTrigger{7});
    world.remove_entity(a);
    world.process_deferred_changes();
    triggers.dispatch(world);
    y_test_assert(values.size() == 3);

    triggers.unsubscribe<TestTrigger>(a, &handler);
    y_test_assert(!triggers.is_listened<TestTrigger>(a));

    // Unsubscribing twice, or from a type without queue is fine
    triggers.unsubscribe<TestTrigger>(a, &handler);
    triggers.unsubscribe<OtherTestTrigger>(a, &handler);
}

y_test_func("TriggerManager emit during dispatch") {
    EntityWorld world;
    TriggerManager& triggers = world.triggers();

    const EntityId a = world.create_entity();
    world.process_deferred_changes();

    core::Vector<u32> values;
    core::Vector<u32> other_values;
    TriggerCallback<TestTrigger> handler([&](EntityWorld& w, EntityId id, const TestTrigger& t) {
        values << t.value;
        if(t.value < 10) {
            w.triggers().emit(id, TestTrigger{t.value + 10});
            w.triggers().emit(id, OtherTestTrigger{t.value});
        }
    });
    TriggerCallback<OtherTestTrigger> other_handler([&](EntityWorld&, EntityId, const OtherTestTrigger& t) {
        other_values << t.value;
    });
    triggers.subscribe<TestTrigger>(a, &handler);
    triggers.subscribe<OtherTestTrigger>(a, &other_handler);

    for(u32 i = 0; i != 4; ++i) {
        triggers.emit(a, TestTrigger{i});
    }

    // Delivered in emission order
    triggers.dispatch(world);
    y_test_assert((values == core::Vector<u32>{0, 1, 2, 3}));
    y_test_assert(other_values.is_empty());

    // Triggers emitted by handlers are delivered by the next dispatch, whatever their type
    triggers.dispatch(world);
    y_test_assert((values == core::Vector<u32>{0, 1, 2, 3, 10, 11, 12, 13}));
    y_test_assert((other_values == core::Vector<u32>{0, 1, 2, 3}));

    triggers.dispatch(world);
    y_test_assert(values.size() == 8);
    y_test_assert(other_values.size() == 4);
}

y_test_func("TriggerManager unsubscribe during dispatch") {
    EntityWorld world;
    TriggerManager& triggers = world.triggers();

    const EntityId a = world.create_entity();
    const EntityId b = world.create_entity();
    world.process_deferred_changes();

    // Handlers that unsubscribe themselves when called
    usize first_calls = 0;
    TriggerHandler* first_ptr = nullptr;
    TriggerCallback<TestTrigger> first([&](EntityWorld& w, EntityId id, const TestTrigger&) {
        ++first_calls;
        w.triggers().unsubscribe<TestTrigger>(id, first_ptr);
    });
    first_ptr = &first;

    core::Vector<u32> second_values;
    TriggerCallback<TestTrigger> second([&](EntityWorld&, EntityId, const TestTrigger& t) {
        second_values << t.value;
    });

    // Other handlers of the same target are still called
    triggers.subscribe<TestTrigger>(a, &first);
    triggers.subscribe<TestTrigger>(a, &second);

    triggers.emit(a, TestTrigger{1});
    triggers.dispatch(world);
    y_test_assert(first_calls == 1);
    y_test_assert((second_values == core::Vector<u32>{1}));

    triggers.emit(a, TestTrigger{2});
    triggers.dispatch(world);
    y_test_assert(first_calls == 1);
    y_test_assert((second_values == core::Vector<u32>{1, 2}));

    // Last handler of a target, other targets are not affected
    first_calls = 0;
    triggers.subscribe<TestTrigger>(b, &first);
    triggers.emit(b, TestTrigger{3});
    triggers.emit(a, TestTrigger{4});
    triggers.dispatch(world);
    y_test_assert(first_calls == 1);
    y_test_assert(!triggers.is_listened<TestTrigger>(b));
    y_test_assert(triggers.is_listened<TestTrigger>(a));
    y_test_assert((second_values == core::Vector<u32>{1, 2, 4}));
}

y_test_func("TriggerManager concurrent emit") {
    EntityWorld world;
    TriggerManager& triggers = world.triggers();

    const EntityId a = world.create_entity();
    world.process_deferred_changes();

    usize count = 0;
    u64 sum = 0;
    TriggerCallback<TestTrigger> handler([&](EntityWorld&, EntityId, const TestTrigger& t) {
        ++count;
        sum += t.value;
    });
    triggers.subscribe<TestTrigger>(a, &handler);

    const usize thread_count = 4;
    const u32 per_thread = 2000;
    {
        core::Vector<std::thread> threads;
        for(usize i = 0; i != thread_count; ++i) {
            threads.emplace_back([&] {
                for(u32 k = 0; k != per_thread; ++k) {
                    triggers.emit(a, TestTrigger{k});
                }
            });
        }
        for(std::thread& thread : threads) {
            thread.join();
        }
    }

    triggers.dispatch(world);
    y_test_assert(count == thread_count * per_thread);
    y_test_assert(sum == u64(thread_count) * (u64(per_thread) * (per_thread - 1) / 2));
}

y_test_func("EntityWorld blueprint component changes") {
    TestOutputBlueprintNode::outputs = {};

    concurrent::JobSystem job_system(2);
    EntityWorld world;
    world.add_system<TriggerSystem>();

    const auto tick = [&] {
        world.tick(job_system);
        world.process_deferred_changes();
    };

    const TriggerManager& triggers = world.triggers();

    const EntityId id = world.create_entity();
    world.add_or_replace_component<BlueprintComponent>(id, make_asset<Blueprint>(make_trigger_output_blueprint<BlueprintTestTrigger>(0)));
    tick();
    tick();
    y_test_assert(triggers.is_listened<BlueprintTestTrigger>(id));
    y_test_assert(!triggers.is_listened<OtherBlueprintTestTrigger>(id));

    world.triggers().emit(id, BlueprintTestTrigger{1.0f});
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 1.0f);

    // Replacing the blueprint replaces the subscriptions
    world.add_or_replace_component<BlueprintComponent>(id, make_asset<Blueprint>(make_trigger_output_blueprint<OtherBlueprintTestTrigger>(1)));
    tick();
    tick();
    y_test_assert(!triggers.is_listened<BlueprintTestTrigger>(id));
    y_test_assert(triggers.is_listened<OtherBlueprintTestTrigger>(id));

    world.triggers().emit(id, BlueprintTestTrigger{3.0f});
    world.triggers().emit(id, OtherBlueprintTestTrigger{2.0f});
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 1.0f);
    y_test_assert(TestOutputBlueprintNode::outputs[1] == 2.0f);

    // Invalid blueprints have no instance and no subscriptions
    {
        Blueprint invalid;
        invalid.add_node(create_blueprint_node("If"));
        world.add_or_replace_component<BlueprintComponent>(id, make_asset<Blueprint>(std::move(invalid)));
    }
    tick();
    tick();
    y_test_assert(!world.component<BlueprintComponent>(id)->instance());
    y_test_assert(!triggers.is_listened<BlueprintTestTrigger>(id));
    y_test_assert(!triggers.is_listened<OtherBlueprintTestTrigger>(id));

    // Empty component
    world.add_or_replace_component<BlueprintComponent>(id);
    tick();
    tick();
    y_test_assert(!world.component<BlueprintComponent>(id)->instance());
    y_test_assert(!triggers.is_listened<BlueprintTestTrigger>(id));

    // Runtime errors are reported but don't break the component
    {
        Blueprint blueprint;
        const BlueprintNode* on = blueprint.add_node(std::make_unique<TriggerBlueprintNode<BlueprintTestTrigger>>("on a"));
        const BlueprintNode* div = blueprint.add_node(create_blueprint_node("Divide float"));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(2));
        blueprint.add_link(on, 0, div, 1);
        blueprint.add_link(div, 0, out, 0);
        world.add_or_replace_component<BlueprintComponent>(id, make_asset<Blueprint>(std::move(blueprint)));
    }
    tick();
    tick();
    y_test_assert(triggers.is_listened<BlueprintTestTrigger>(id));
    world.triggers().emit(id, BlueprintTestTrigger{0.0f});
    tick();
    y_test_assert(world.component<BlueprintComponent>(id)->instance());
    world.triggers().emit(id, BlueprintTestTrigger{-1.0f});
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[2] == 0.0f);

    // Removing the component unsubscribes
    world.remove_component<BlueprintComponent>(id);
    tick();
    tick();
    y_test_assert(!triggers.is_listened<BlueprintTestTrigger>(id));
}

y_test_func("EntityWorld blueprint component save and load") {
    const AssetId asset_id = AssetId::from_id(1234);

    EntityWorld source;
    const EntityId id = source.create_entity();
    source.add_or_replace_component<BlueprintComponent>(id, make_asset_with_id<Blueprint>(asset_id, make_trigger_output_blueprint<BlueprintTestTrigger>(0)));
    source.process_deferred_changes();

    concurrent::JobSystem job_system(2);
    EntityWorld world;
    world.add_system<TriggerSystem>();
    save_and_load(source, world);

    const BlueprintComponent* component = world.component<BlueprintComponent>(id);
    y_test_assert(component);
    y_test_assert(component->blueprint().id() == asset_id);

    // No loader: the asset is not loaded, nothing should be subscribed
    world.tick(job_system);
    world.process_deferred_changes();
    world.tick(job_system);
    world.process_deferred_changes();
    y_test_assert(!component->instance());
    y_test_assert(!world.triggers().is_listened<BlueprintTestTrigger>(id));
}

y_test_func("EntityWorld blueprint component on collide") {
    TestOutputBlueprintNode::outputs = {};

    // On collide -> Decompose Vec3 -> output(y)
    Blueprint blueprint;
    {
        const BlueprintNode* on_collide = blueprint.add_node(create_blueprint_node("On collide"));
        const BlueprintNode* decompose = blueprint.add_node(create_blueprint_node("Decompose Vec3"));
        const BlueprintNode* out = blueprint.add_node(std::make_unique<TestOutputBlueprintNode>(0));
        y_test_assert(on_collide->output_pins().size() == 2);
        blueprint.add_link(on_collide, 1, decompose, 0);
        blueprint.add_link(decompose, 1, out, 0);
    }

    concurrent::JobSystem job_system(2);
    EntityWorld world;
    world.add_system<TriggerSystem>();

    const auto tick = [&] {
        world.tick(job_system);
        world.process_deferred_changes();
    };

    const EntityId id = world.create_entity();
    const EntityId other = world.create_entity();
    world.add_or_replace_component<BlueprintComponent>(id, make_asset<Blueprint>(std::move(blueprint)));
    tick();
    tick();
    y_test_assert(world.triggers().is_listened<OnCollide>(id));
    y_test_assert(!world.triggers().is_listened<OnCollide>(other));

    world.triggers().emit(id, OnCollide{other, math::Vec3(1.0f, 2.0f, 3.0f)});
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 2.0f);

    // Triggers pending on a removed entity are dropped
    world.triggers().emit(id, OnCollide{other, math::Vec3(1.0f, 5.0f, 3.0f)});
    world.remove_entity(id);
    world.process_deferred_changes();
    tick();
    y_test_assert(TestOutputBlueprintNode::outputs[0] == 2.0f);
}



// ---------------------------------------- Time ----------------------------------------

y_test_func("TimeSystem") {
    concurrent::JobSystem job_system(2);
    EntityWorld world;

    y_test_assert(TimeSystem::dt(world) == 0.0f);

    TimeSystem* time = world.add_system<TimeSystem>(2.0f);
    y_test_assert(time->time_scale() == 2.0f);
    time->set_time_scale(0.5f);
    y_test_assert(time->time_scale() == 0.5f);

    world.tick(job_system);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    world.tick(job_system);

    y_test_assert(time->dt() > 0.0f);
    y_test_assert(TimeSystem::dt(world) == time->dt());

    // Negative and null scales never produce negative time
    time->set_time_scale(-1.0f);
    y_test_assert(time->dt() == 0.0f);
    time->set_time_scale(0.0f);
    y_test_assert(time->dt() == 0.0f);
}



// ---------------------------------------- AABB ----------------------------------------

bool nearly_equal(const math::Vec3& a, const math::Vec3& b, float eps = 0.0001f) {
    return (a - b).length() <= eps;
}

bool same_aabb(const AABB& a, const AABB& b) {
    return a.min() == b.min() && a.max() == b.max();
}

y_test_func("AABB basics") {
    const AABB aabb(math::Vec3(1.0f, 2.0f, 3.0f), math::Vec3(-1.0f, 0.0f, 5.0f));
    y_test_assert(aabb.min() == math::Vec3(-1.0f, 0.0f, 3.0f));
    y_test_assert(aabb.max() == math::Vec3(1.0f, 2.0f, 5.0f));
    y_test_assert(aabb.center() == math::Vec3(0.0f, 1.0f, 4.0f));
    y_test_assert(aabb.extent() == math::Vec3(2.0f, 2.0f, 2.0f));
    y_test_assert(aabb.half_extent() == math::Vec3(1.0f, 1.0f, 1.0f));
    y_test_assert(std::abs(aabb.radius() - std::sqrt(3.0f)) < 0.0001f);
    y_test_assert(std::abs(aabb.origin_radius() - std::sqrt(1.0f + 4.0f + 25.0f)) < 0.0001f);
    y_test_assert(!aabb.is_empty());

    const AABB centered = AABB::from_center_extent(math::Vec3(1.0f, 1.0f, 1.0f), math::Vec3(2.0f, 4.0f, 6.0f));
    y_test_assert(centered.min() == math::Vec3(0.0f, -1.0f, -2.0f));
    y_test_assert(centered.max() == math::Vec3(2.0f, 3.0f, 4.0f));
    y_test_assert(centered.center() == math::Vec3(1.0f, 1.0f, 1.0f));

    y_test_assert(AABB().is_empty());
    y_test_assert(AABB(math::Vec3(4.0f), math::Vec3(4.0f)).is_empty());
    y_test_assert(!AABB(math::Vec3(0.0f), math::Vec3(1.0f, 1.0f, 0.0f)).is_empty());

    const AABB merged = aabb.merged(centered);
    y_test_assert(merged.min() == math::Vec3(-1.0f, -1.0f, -2.0f));
    y_test_assert(merged.max() == math::Vec3(2.0f, 3.0f, 5.0f));
    y_test_assert(merged.contains(aabb));
    y_test_assert(merged.contains(centered));
}

y_test_func("AABB contains") {
    const AABB aabb(math::Vec3(0.0f), math::Vec3(1.0f));

    y_test_assert(aabb.contains(math::Vec3(0.5f)));
    y_test_assert(aabb.contains(math::Vec3(0.0f)));
    y_test_assert(aabb.contains(math::Vec3(1.0f)));
    y_test_assert(!aabb.contains(math::Vec3(1.5f, 0.5f, 0.5f)));
    y_test_assert(!aabb.contains(math::Vec3(0.5f, -0.1f, 0.5f)));
    y_test_assert(!aabb.contains(math::Vec3(0.5f, 0.5f, 2.0f)));

    y_test_assert(aabb.contains(aabb));
    y_test_assert(aabb.contains(AABB(math::Vec3(0.25f), math::Vec3(0.75f))));
    y_test_assert(!aabb.contains(AABB(math::Vec3(0.5f), math::Vec3(1.5f))));
    y_test_assert(!aabb.contains(AABB(math::Vec3(-1.0f), math::Vec3(2.0f))));
    y_test_assert(!aabb.contains(AABB(math::Vec3(2.0f), math::Vec3(3.0f))));
}



// ---------------------------------------- TransformableComponent ----------------------------------------

y_test_func("TransformableComponent basics") {
    TransformableComponent tr;
    y_test_assert(tr.position() == math::Vec3(0.0f));
    y_test_assert(nearly_equal(tr.forward(), math::Vec3(0.0f, 0.0f, 1.0f)));
    y_test_assert(std::abs(tr.right().length() - 1.0f) < 0.0001f);
    y_test_assert(std::abs(tr.up().length() - 1.0f) < 0.0001f);
    y_test_assert(std::abs(tr.forward().dot(tr.right())) < 0.0001f);
    y_test_assert(std::abs(tr.forward().dot(tr.up())) < 0.0001f);
    y_test_assert(std::abs(tr.right().dot(tr.up())) < 0.0001f);

    tr.set_position(math::Vec3(1.0f, 2.0f, 3.0f));
    y_test_assert(tr.position() == math::Vec3(1.0f, 2.0f, 3.0f));
    y_test_assert(tr.to_global(math::Vec3(1.0f, 0.0f, 0.0f)) == math::Vec3(2.0f, 2.0f, 3.0f));

    // Rotation and scale
    const auto rotation = math::Quaternion<>::from_axis_angle(math::Vec3(0.0f, 0.0f, 1.0f), math::to_rad(90.0f));
    tr.set_transform(math::Transform<>(math::Vec3(1.0f, 2.0f, 3.0f), rotation, math::Vec3(2.0f)));
    y_test_assert(tr.position() == math::Vec3(1.0f, 2.0f, 3.0f));
    y_test_assert(nearly_equal(tr.forward().normalized(), rotation(math::Vec3(0.0f, 0.0f, 1.0f))));

    const math::Vec3 p(1.0f, 2.0f, 3.0f);
    y_test_assert(nearly_equal(tr.to_global(p), math::Vec3(1.0f, 2.0f, 3.0f) + rotation(p) * 2.0f));

    // Position change keeps rotation
    tr.set_position(math::Vec3(0.0f));
    y_test_assert(nearly_equal(tr.to_global(p), rotation(p) * 2.0f));
}

y_test_func("TransformableComponent global AABB") {
    const auto check = [](const math::Transform<>& transform, const AABB& local) {
        const TransformableComponent tr(transform);
        const AABB global = tr.to_global(local);

        math::Vec3 corner_min(std::numeric_limits<float>::max());
        math::Vec3 corner_max(-std::numeric_limits<float>::max());
        for(usize i = 0; i != 8; ++i) {
            const math::Vec3 corner(
                (i & 1) ? local.max().x() : local.min().x(),
                (i & 2) ? local.max().y() : local.min().y(),
                (i & 4) ? local.max().z() : local.min().z()
            );
            const math::Vec3 p = tr.to_global(corner);
            corner_min = corner_min.min(p);
            corner_max = corner_max.max(p);
        }

        // Must contain every transformed corner and be tight
        return nearly_equal(global.min(), corner_min, 0.001f) && nearly_equal(global.max(), corner_max, 0.001f);
    };

    const AABB local(math::Vec3(-1.0f, 0.0f, 2.0f), math::Vec3(3.0f, 1.0f, 4.0f));

    y_test_assert(check(math::Transform<>(), local));
    y_test_assert(check(math::Transform<>(math::Vec3(5.0f, -3.0f, 2.0f)), local));
    y_test_assert(check(math::Transform<>(math::Vec3(0.0f), math::Quaternion<>::from_axis_angle(math::Vec3(0.0f, 0.0f, 1.0f), math::to_rad(45.0f))), local));
    y_test_assert(check(math::Transform<>(math::Vec3(1.0f, 2.0f, 3.0f), math::Quaternion<>::from_euler(math::to_rad(30.0f), math::to_rad(60.0f), math::to_rad(-20.0f)), math::Vec3(2.0f, 0.5f, 3.0f)), local));
    y_test_assert(check(math::Transform<>(math::Vec3(0.0f), math::Quaternion<>(), math::Vec3(-1.0f, 1.0f, 1.0f)), local));
}

y_test_func("TransformableComponent save and load") {
    EntityWorld world;
    const EntityId id = world.create_entity();
    const math::Transform<> transform(math::Vec3(1.0f, 2.0f, 3.0f), math::Quaternion<>::from_euler(0.1f, 0.2f, 0.3f), math::Vec3(4.0f));
    world.add_or_replace_component<TransformableComponent>(id, transform);
    world.process_deferred_changes();

    EntityWorld loaded;
    save_and_load(world, loaded);

    const TransformableComponent* tr = loaded.component<TransformableComponent>(id);
    y_test_assert(tr);
    y_test_assert(tr->transform() == transform);
}



// ---------------------------------------- IndexAllocator ----------------------------------------

y_test_func("IndexAllocator") {
    IndexAllocator<u32> allocator;
    y_test_assert(allocator.size() == 0);

    y_test_assert(allocator.alloc() == 0);
    y_test_assert(allocator.alloc() == 1);
    y_test_assert(allocator.alloc() == 2);
    y_test_assert(allocator.size() == 3);

    allocator.free(1);
    y_test_assert(allocator.size() == 2);
    allocator.free(0);
    y_test_assert(allocator.size() == 1);

    // Freed indices are reused before new ones
    const u32 a = allocator.alloc();
    const u32 b = allocator.alloc();
    y_test_assert(a != b);
    y_test_assert(a <= 1 && b <= 1);
    y_test_assert(allocator.size() == 3);

    y_test_assert(allocator.alloc() == 3);
    y_test_assert(allocator.size() == 4);
}



// ---------------------------------------- SpatialPartition ----------------------------------------

y_test_func("SpatialPartition random operations") {
    SpatialPartition<u32> partition;
    std::unordered_map<u32, AABB> expected;

    std::mt19937 rng(42);
    const auto rand_float = [&](float a, float b) {
        return std::uniform_real_distribution<float>(a, b)(rng);
    };
    const auto random_aabb = [&] {
        const float range = std::pow(10.0f, rand_float(-1.0f, 3.0f));
        const math::Vec3 center(rand_float(-range, range), rand_float(-range, range), rand_float(-range, range));
        const float size = std::pow(10.0f, rand_float(-2.0f, 1.5f));
        const math::Vec3 extent(rand_float(0.0f, size), rand_float(0.0f, size), rand_float(0.0f, size));
        return AABB::from_center_extent(center, extent);
    };

    u32 next_value = 0;
    for(usize i = 0; i != 4000; ++i) {
        const u32 op = u32(rng() % 5);
        if(op < 2 || !partition.size()) {
            const AABB aabb = random_aabb();
            const u32* inserted = partition.insert(aabb, next_value);
            y_test_assert(*inserted == next_value);
            expected[next_value++] = aabb;
        } else if(op == 2) {
            // Small move
            const usize index = rng() % partition.size();
            const AABB old = partition.aabb(&partition[index]);
            const AABB aabb = AABB::from_center_extent(old.center() + math::Vec3(rand_float(-0.1f, 0.1f), 0.0f, 0.0f), old.extent());
            partition.update(&partition[index], aabb);
            expected[partition[index]] = aabb;
        } else if(op == 3) {
            // Teleport
            const usize index = rng() % partition.size();
            const AABB aabb = random_aabb();
            partition.update(&partition[index], aabb);
            expected[partition[index]] = aabb;
        } else {
            const usize index = rng() % partition.size();
            const u32 value = partition[index];
            partition.erase_unordered(&partition[index]);
            expected.erase(value);
        }

        if(i % 50 == 0) {
            y_test_assert(partition.size() == expected.size());
            y_test_assert(partition.values().size() == partition.size());
            for(const u32& value : partition) {
                const auto it = expected.find(value);
                y_test_assert(it != expected.end());
                y_test_assert(same_aabb(partition.aabb(&value), it->second));
                y_test_assert(partition.cell_aabb(&value).contains(partition.aabb(&value)));
            }
        }
    }

    // Erase everything
    while(partition.size()) {
        const usize index = rng() % partition.size();
        expected.erase(partition[index]);
        partition.erase_unordered(&partition[index]);
    }
    y_test_assert(expected.empty());
    y_test_assert(partition.begin() == partition.end());

    // Still usable
    partition.emplace_back(7u);
    y_test_assert(partition.size() == 1);
    y_test_assert(partition[0] == 7);
    y_test_assert(partition.aabb(&partition[0]).is_empty());
}

y_test_func("SpatialPartition cells follow object size") {
    SpatialPartition<u32> partition;

    const math::Vec3 positions[] = {
        math::Vec3(1.0f, 0.0f, 0.0f),
        math::Vec3(1000.0f, 0.0f, 0.0f),
        math::Vec3(-1000.0f, 0.0f, 0.0f),
        math::Vec3(0.0f, 1000.0f, 1000.0f),
    };
    for(u32 i = 0; i != 4; ++i) {
        partition.insert(AABB::from_center_extent(positions[i], math::Vec3(1.0f)), i);
    }

    // Cells should be sized after the object, not its distance to the origin
    for(const u32& value : partition) {
        const AABB cell = partition.cell_aabb(&value);
        y_test_assert(cell.contains(partition.aabb(&value)));
        y_test_assert(cell.extent().x() < 16.0f);
    }

    // Small objects far apart don't share a cell
    for(usize i = 0; i != 4; ++i) {
        for(usize j = i + 1; j != 4; ++j) {
            y_test_assert(!nearly_equal(partition.cell_aabb(&partition[i]).center(), partition.cell_aabb(&partition[j]).center(), 1.0f));
        }
    }
}



// ---------------------------------------- File systems ----------------------------------------

// Don't unwrap errors in asserts: it would abort all tests
template<typename R, typename T>
bool ok_eq(R&& res, const T& value) {
    return res.is_ok() && res.unwrap() == value;
}

template<typename R>
bool ok_size(R&& res, usize size) {
    return res.is_ok() && res.unwrap().size() == size;
}

template<typename R, typename T>
bool ok_first(R&& res, const T& value) {
    return res.is_ok() && !res.unwrap().is_empty() && res.unwrap()[0] == value;
}

class TempDirectory : NonCopyable {
    public:
        TempDirectory(std::string_view name) {
            std::random_device rd;
            _path = std::filesystem::temp_directory_path() / (std::string("yave_tests_") + std::string(name) + "_" + std::to_string(rd()));
            std::filesystem::create_directories(_path);
        }

        ~TempDirectory() {
            std::error_code ec;
            std::filesystem::remove_all(_path, ec);
        }

        core::String path() const {
            return core::String(_path.generic_string());
        }

        core::String file(std::string_view name) const {
            return core::String((_path / std::string(name)).generic_string());
        }

    private:
        std::filesystem::path _path;
};

void write_file(const core::String& path, std::string_view content) {
    std::ofstream file(std::string(std::string_view(path)), std::ios::binary);
    file.write(content.data(), std::streamsize(content.size()));
}

core::Vector<core::String> list_entries(const FileSystemModel* fs, std::string_view path) {
    core::Vector<core::String> names;
    if(fs->for_each(path, [&](const FileSystemModel::EntryInfo& info) { names << info.name; }).is_error()) {
        names << "<error>";
    }
    return names;
}

bool has_name(const core::Vector<core::String>& names, std::string_view name) {
    return std::find_if(names.begin(), names.end(), [&](const core::String& n) { return std::string_view(n) == name; }) != names.end();
}

y_test_func("LocalFileSystemModel paths") {
    const LocalFileSystemModel fs{};

    y_test_assert(fs.join("a", "b") == "a/b");
    y_test_assert(fs.join("a/", "b") == "a/b");
    y_test_assert(fs.join("a\\", "b") == "a\\b");
    y_test_assert(fs.join("", "b") == "b");

    y_test_assert(fs.filename("a/b/c.txt") == "c.txt");
    y_test_assert(fs.filename("c.txt") == "c.txt");

    y_test_assert(fs.extension("a/b/c.txt") == ".txt");
    y_test_assert(fs.extension("c.tar.gz") == ".gz");
    y_test_assert(fs.extension("noext") == "");
    y_test_assert(fs.extension("dir.d/file") == "");

    y_test_assert(fs.is_delimiter('/'));
    y_test_assert(fs.is_delimiter('\\'));
    y_test_assert(!fs.is_delimiter('a'));

    y_test_assert(fs.canonicalize("a\\b\\..\\c") == "a/c");
    y_test_assert(fs.canonicalize("a/./b") == "a/b");
    y_test_assert(fs.is_canonical("a/c"));
    y_test_assert(!fs.is_canonical("a/b/../c"));

    // Views that aren't null terminated
    const std::string_view full = "a/b/../cXYZ";
    y_test_assert(fs.canonicalize(full.substr(0, 8)) == "a/c");
    y_test_assert(!fs.is_canonical(full.substr(0, 8)));

    {
        const auto abs = fs.absolute("some/rel/../path");
        y_test_assert(abs.is_ok());
        y_test_assert(std::find(abs.unwrap().begin(), abs.unwrap().end(), '\\') == abs.unwrap().end());
        y_test_assert(abs.unwrap().ends_with("some/path"));
    }

    y_test_assert(fs.current_path().is_ok());
    y_test_assert(FileSystemModel::local_filesystem());
}

y_test_func("LocalFileSystemModel operations") {
    const TempDirectory temp("localfs");
    const FileSystemModel* fs = FileSystemModel::local_filesystem();

    const core::String dir = temp.file("dir");
    const core::String sub = temp.file("dir/sub");
    const core::String file = temp.file("dir/file.txt");

    y_test_assert(!fs->exists(dir).unwrap_or(true));
    y_test_assert(ok_eq(fs->entry_type(dir), FileSystemModel::EntryType::Unknown));

    y_test_assert(fs->create_directory(dir).is_ok());
    y_test_assert(fs->create_directory(sub).is_ok());
    write_file(file, "hello");

    y_test_assert(fs->exists(dir).unwrap_or(false));
    y_test_assert(fs->is_directory(dir).unwrap_or(false));
    y_test_assert(!fs->is_file(dir).unwrap_or(true));
    y_test_assert(fs->is_file(file).unwrap_or(false));
    y_test_assert(!fs->is_directory(file).unwrap_or(true));

    {
        usize count = 0;
        bool found_file = false;
        y_test_assert(fs->for_each(dir, [&](const FileSystemModel::EntryInfo& info) {
            ++count;
            if(info.name == "file.txt") {
                found_file = info.type == FileSystemModel::EntryType::File && info.file_size == 5;
            }
        }).is_ok());
        y_test_assert(count == 2);
        y_test_assert(found_file);
        y_test_assert(has_name(list_entries(fs, dir), "sub"));
    }
    y_test_assert(fs->for_each(temp.file("missing"), [](const auto&) {}).is_error());

    // Parents
    y_test_assert(fs->is_parent(dir, sub).unwrap_or(false));
    y_test_assert(fs->is_parent(dir, file).unwrap_or(false));
    y_test_assert(!fs->is_parent(sub, dir).unwrap_or(true));
    y_test_assert(!fs->is_parent(dir, dir).unwrap_or(true));
    y_test_assert(!fs->is_parent(temp.file("di"), dir).unwrap_or(true));
    y_test_assert(ok_eq(fs->parent_path(sub), fs->absolute(dir).unwrap_or("")));

    // Rename
    const core::String renamed = temp.file("dir/renamed.txt");
    y_test_assert(fs->rename(file, renamed).is_ok());
    y_test_assert(!fs->exists(file).unwrap_or(true));
    y_test_assert(fs->is_file(renamed).unwrap_or(false));

    // Rename replaces
    write_file(file, "other");
    y_test_assert(fs->rename(file, renamed).is_ok());
    y_test_assert(!fs->exists(file).unwrap_or(true));
    y_test_assert(fs->exists(renamed).unwrap_or(false));

    y_test_assert(fs->rename(temp.file("missing"), temp.file("missing2")).is_error());

    // Recursive remove
    y_test_assert(fs->remove(dir).is_ok());
    y_test_assert(!fs->exists(dir).unwrap_or(true));
    y_test_assert(!fs->exists(renamed).unwrap_or(true));
}



// ---------------------------------------- FolderAssetStore ----------------------------------------

io2::Buffer make_buffer(std::string_view text) {
    io2::Buffer buffer;
    buffer.write_array(text.data(), text.size()).ignore();
    buffer.reset();
    return buffer;
}

core::Result<AssetId, AssetStore::ErrorType> import_text(AssetStore& store, std::string_view name, std::string_view text, AssetType type = AssetType::Mesh, core::Span<AssetId> refs = {}) {
    io2::Buffer buffer = make_buffer(text);
    return store.import(buffer, name, type, refs);
}

core::String read_asset(const AssetStore& store, AssetId id) {
    auto data = store.data(id);
    if(data.is_error()) {
        return "<error>";
    }
    core::Vector<u8> bytes;
    if(data.unwrap()->read_all(bytes).is_error()) {
        return "<read error>";
    }
    return core::String(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

template<typename T>
bool is_store_error(const AssetStore::Result<T>& res, AssetStore::ErrorType error) {
    return res.is_error() && res.error() == error;
}

y_test_func("AssetId strings") {
    const AssetId id = AssetId::from_id(0x0123456789abcdef);
    const core::String str = stringify_id(id);
    y_test_assert(str.size() == 16);
    y_test_assert(ok_eq(parse_id(str), id));
    y_test_assert(stringify_id(AssetId::from_id(1)) == "0000000000000001");
    y_test_assert(parse_id("zz").is_error());

    const AssetId a = make_random_id();
    const AssetId b = make_random_id();
    y_test_assert(a != b);
    y_test_assert(a != AssetId::invalid_id());
}

y_test_func("FolderAssetStore import and read") {
    const TempDirectory temp("store_import");
    FolderAssetStore store(temp.path());

    const auto one = import_text(store, "folder/one", "hello");
    y_test_assert(one.is_ok());
    const AssetId id1 = one.unwrap();
    y_test_assert(id1 != AssetId::invalid_id());

    y_test_assert(ok_eq(store.id("folder/one"), id1));
    y_test_assert(ok_eq(store.name(id1), "folder/one"));
    y_test_assert(ok_eq(store.asset_type(id1), AssetType::Mesh));
    y_test_assert(read_asset(store, id1) == "hello");
    y_test_assert(ok_size(store.references(id1), 0));
    y_test_assert(store.filesystem()->is_directory("folder").unwrap_or(false));

    // Invalid references are dropped
    const std::array<AssetId, 2> refs = {id1, AssetId::invalid_id()};
    const auto two = import_text(store, "two", "world", AssetType::Image, refs);
    y_test_assert(two.is_ok());
    const AssetId id2 = two.unwrap();
    y_test_assert(id2 != id1);
    y_test_assert(ok_eq(store.asset_type(id2), AssetType::Image));
    y_test_assert(ok_size(store.references(id2), 1));
    y_test_assert(ok_first(store.references(id2), id1));
    y_test_assert(ok_eq(store.reference_count(id1), 1));
    y_test_assert(ok_eq(store.reference_count(id2), 0));
    {
        core::Vector<AssetId> found;
        y_test_assert(store.search_references(id1, [&](AssetId id) { found << id; }).is_ok());
        y_test_assert(found.size() == 1 && found[0] == id2);
    }

    // Errors
    y_test_assert(is_store_error(import_text(store, "folder/one", "again"), AssetStore::ErrorType::NameAlreadyExists));
    y_test_assert(read_asset(store, id1) == "hello");
    y_test_assert(is_store_error(import_text(store, "bad\\name", "x"), AssetStore::ErrorType::InvalidName));
    y_test_assert(is_store_error(import_text(store, "bad\nname", "x"), AssetStore::ErrorType::InvalidName));
    y_test_assert(is_store_error(import_text(store, "", "x"), AssetStore::ErrorType::InvalidName));
    y_test_assert(import_text(store, "folder", "x").is_error());
    y_test_assert(import_text(store, "two/child", "x").is_error());

    const AssetId unknown = AssetId::from_id(12345);
    y_test_assert(is_store_error(store.name(unknown), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.id("missing"), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.data(unknown), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.asset_type(unknown), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.references(unknown), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.remove(unknown), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.rename(unknown, "x"), AssetStore::ErrorType::UnknownID));
    y_test_assert(is_store_error(store.name(AssetId::invalid_id()), AssetStore::ErrorType::UnknownID));
}

y_test_func("FolderAssetStore write") {
    const TempDirectory temp("store_write");
    FolderAssetStore store(temp.path());

    const AssetId id1 = import_text(store, "one", "hello").unwrap();
    const AssetId id2 = import_text(store, "two", "world").unwrap();

    {
        io2::Buffer buffer = make_buffer("changed!");
        const std::array<AssetId, 1> refs = {id2};
        y_test_assert(store.write(id1, buffer, refs).is_ok());
    }
    y_test_assert(read_asset(store, id1) == "changed!");
    y_test_assert(ok_size(store.references(id1), 1));
    y_test_assert(ok_first(store.references(id1), id2));
    y_test_assert(ok_eq(store.reference_count(id2), 1));
    y_test_assert(ok_eq(store.name(id1), "one"));
    y_test_assert(ok_eq(store.asset_type(id1), AssetType::Mesh));

    // File size is reported by the filesystem
    {
        bool found = false;
        store.filesystem()->for_each("", [&](const FileSystemModel::EntryInfo& info) {
            if(info.name == "one") {
                found = info.file_size == 8;
            }
        }).ignore();
        y_test_assert(found);
    }

    {
        io2::Buffer buffer = make_buffer("x");
        y_test_assert(is_store_error(store.write(AssetId::from_id(12345), buffer, {}), AssetStore::ErrorType::UnknownID));
        io2::Buffer buffer2 = make_buffer("x");
        y_test_assert(is_store_error(store.write(AssetId::invalid_id(), buffer2, {}), AssetStore::ErrorType::UnknownID));
    }
}

y_test_func("FolderAssetStore rename and remove") {
    const TempDirectory temp("store_rename");
    FolderAssetStore store(temp.path());

    const AssetId id1 = import_text(store, "a/b/one", "1").unwrap();
    const AssetId id2 = import_text(store, "a/two", "2").unwrap();
    const AssetId id3 = import_text(store, "three", "3").unwrap();

    // Rename asset
    y_test_assert(store.rename(id3, "renamed").is_ok());
    y_test_assert(ok_eq(store.name(id3), "renamed"));
    y_test_assert(ok_eq(store.id("renamed"), id3));
    y_test_assert(store.id("three").is_error());
    y_test_assert(read_asset(store, id3) == "3");

    // Rename onto existing names fails and changes nothing
    y_test_assert(store.rename("renamed", "a/two").is_error());
    y_test_assert(ok_eq(store.name(id3), "renamed"));
    y_test_assert(ok_eq(store.name(id2), "a/two"));
    y_test_assert(store.rename("renamed", "a").is_error());
    y_test_assert(ok_eq(store.name(id3), "renamed"));

    // Rename folder moves everything inside
    y_test_assert(store.rename("a/b", "z").is_ok());
    y_test_assert(ok_eq(store.name(id1), "z/one"));
    y_test_assert(ok_eq(store.id("z/one"), id1));
    y_test_assert(store.id("a/b/one").is_error());
    y_test_assert(ok_eq(store.name(id2), "a/two"));
    y_test_assert(store.filesystem()->is_directory("z").unwrap_or(false));
    y_test_assert(!store.filesystem()->exists("a/b").unwrap_or(true));
    y_test_assert(read_asset(store, id1) == "1");

    // Can't move a folder inside itself
    y_test_assert(store.rename("z", "z/inner").is_error());
    y_test_assert(ok_eq(store.name(id1), "z/one"));

    // Renaming into a folder that doesn't exist creates it
    y_test_assert(store.rename("renamed", "new/dir/renamed").is_ok());
    y_test_assert(ok_eq(store.name(id3), "new/dir/renamed"));
    y_test_assert(store.filesystem()->is_directory("new/dir").unwrap_or(false));
    y_test_assert(store.filesystem()->is_directory("new").unwrap_or(false));
    y_test_assert(has_name(list_entries(store.filesystem(), "new/dir"), "renamed"));

    // Remove by id
    y_test_assert(store.remove(id2).is_ok());
    y_test_assert(store.name(id2).is_error());
    y_test_assert(store.id("a/two").is_error());
    y_test_assert(store.data(id2).is_error());
    y_test_assert(store.filesystem()->is_directory("a").unwrap_or(false));

    // Remove by name
    y_test_assert(store.remove("new/dir/renamed").is_ok());
    y_test_assert(store.name(id3).is_error());

    // Remove folder removes its content
    const AssetId id4 = import_text(store, "z/deep/four", "4").unwrap();
    y_test_assert(store.remove("z").is_ok());
    y_test_assert(store.name(id1).is_error());
    y_test_assert(store.name(id4).is_error());
    y_test_assert(store.data(id1).is_error());
    y_test_assert(!store.filesystem()->exists("z").unwrap_or(true));
    y_test_assert(!store.filesystem()->exists("z/deep").unwrap_or(true));
}

y_test_func("FolderAssetStore reopen") {
    const TempDirectory temp("store_reopen");

    AssetId id1;
    AssetId id2;
    {
        FolderAssetStore store(temp.path());
        id1 = import_text(store, "folder/one", "hello", AssetType::Font).unwrap();
        const std::array<AssetId, 1> refs = {id1};
        id2 = import_text(store, "two", "world", AssetType::Material, refs).unwrap();
        y_test_assert(store.filesystem()->create_directory("empty/nested").is_ok());
    }

    {
        FolderAssetStore store(temp.path());
        y_test_assert(ok_eq(store.id("folder/one"), id1));
        y_test_assert(ok_eq(store.id("two"), id2));
        y_test_assert(ok_eq(store.asset_type(id1), AssetType::Font));
        y_test_assert(ok_eq(store.asset_type(id2), AssetType::Material));
        y_test_assert(read_asset(store, id1) == "hello");
        y_test_assert(ok_size(store.references(id2), 1));
        y_test_assert(ok_first(store.references(id2), id1));
        y_test_assert(store.filesystem()->is_directory("folder").unwrap_or(false));
        y_test_assert(store.filesystem()->is_directory("empty").unwrap_or(false));
        y_test_assert(store.filesystem()->is_directory("empty/nested").unwrap_or(false));

        // Renames are persisted
        // Note: renaming a folder into a non existing parent asserts in save_tree, so create it first
        y_test_assert(store.filesystem()->create_directory("moved").is_ok());
        y_test_assert(store.rename("folder", "moved/folder").is_ok());
    }

    {
        FolderAssetStore store(temp.path());
        y_test_assert(ok_eq(store.name(id1), "moved/folder/one"));
        y_test_assert(store.filesystem()->is_directory("moved/folder").unwrap_or(false));
        y_test_assert(store.filesystem()->is_directory("moved").unwrap_or(false));
    }
}

y_test_func("FolderAssetStore filesystem") {
    const TempDirectory temp("store_fs");
    FolderAssetStore store(temp.path());
    const FileSystemModel* fs = store.filesystem();

    y_test_assert(ok_size(fs->current_path(), 0));
    y_test_assert(fs->exists("").unwrap_or(false));
    y_test_assert(fs->is_directory("").unwrap_or(false));

    y_test_assert(fs->create_directory("a/b/c").is_ok());
    y_test_assert(fs->is_directory("a").unwrap_or(false));
    y_test_assert(fs->is_directory("a/b").unwrap_or(false));
    y_test_assert(fs->is_directory("a/b/c").unwrap_or(false));
    y_test_assert(fs->exists("a/b/").unwrap_or(false));
    y_test_assert(!fs->exists("a/x").unwrap_or(true));

    import_text(store, "a/b/mesh", "data").unwrap();
    import_text(store, "a/b-other", "data").unwrap();
    import_text(store, "root", "data").unwrap();

    y_test_assert(fs->exists("a/b/mesh").unwrap_or(false));
    y_test_assert(fs->is_file("a/b/mesh").unwrap_or(false));
    y_test_assert(!fs->exists("a/b/mesh/").unwrap_or(true));

    y_test_assert(fs->join("a", "b") == "a/b");
    y_test_assert(fs->join("a/", "b") == "a/b");
    y_test_assert(fs->join("", "b") == "b");
    y_test_assert(fs->filename("a/b/mesh") == "mesh");
    y_test_assert(fs->filename("mesh") == "mesh");
    y_test_assert(ok_eq(fs->parent_path("a/b/mesh"), "a/b"));
    y_test_assert(ok_eq(fs->parent_path("a"), ""));
    y_test_assert(fs->is_parent("a", "a/b/mesh").unwrap_or(false));
    y_test_assert(!fs->is_parent("a/b", "a/b-other").unwrap_or(true));

    {
        const auto entries = list_entries(fs, "a/b");
        y_test_assert(entries.size() == 2);
        y_test_assert(has_name(entries, "c"));
        y_test_assert(has_name(entries, "mesh"));
    }
    {
        const auto entries = list_entries(fs, "a/b/");
        y_test_assert(entries.size() == 2);
    }
    {
        const auto entries = list_entries(fs, "a");
        y_test_assert(entries.size() == 2);
        y_test_assert(has_name(entries, "b"));
        y_test_assert(has_name(entries, "b-other"));
    }
    {
        const auto entries = list_entries(fs, "");
        y_test_assert(entries.size() == 2);
        y_test_assert(has_name(entries, "a"));
        y_test_assert(has_name(entries, "root"));
    }
    y_test_assert(list_entries(fs, "missing").is_empty());

    const auto search = [&](std::string_view path, std::string_view pattern) {
        core::Vector<core::String> found;
        dynamic_cast<const SearchableFileSystemModel*>(fs)->search(path, pattern, [&](const FileSystemModel::EntryInfo& info) { found << info.name; }).ignore();
        return found;
    };

    y_test_assert(dynamic_cast<const SearchableFileSystemModel*>(fs));
    {
        const auto found = search("", "mes");
        y_test_assert(found.size() == 1 && found[0] == "a/b/mesh");
    }
    {
        const auto found = search("a", "b");
        y_test_assert(has_name(found, "a/b"));
        y_test_assert(has_name(found, "a/b/c"));
        y_test_assert(has_name(found, "a/b/mesh"));
        y_test_assert(has_name(found, "a/b-other"));
    }
    {
        const auto found = search("a/b", "o");
        y_test_assert(found.is_empty());
    }
    y_test_assert(search("missing", "a").is_empty());

    // Directory operations
    y_test_assert(fs->rename("a/b/c", "a/d").is_ok());
    y_test_assert(fs->is_directory("a/d").unwrap_or(false));
    y_test_assert(!fs->exists("a/b/c").unwrap_or(true));
    y_test_assert(fs->rename("a/d", "").is_error());
    y_test_assert(fs->rename("a/d", "bad\\name").is_error());

    y_test_assert(fs->remove("a/d").is_ok());
    y_test_assert(!fs->exists("a/d").unwrap_or(true));
    y_test_assert(fs->exists("a/b/mesh").unwrap_or(false));
}



// ---------------------------------------- Stress ----------------------------------------

y_test_func("EntityWorld randomized operations") {
    struct State {
        bool has_pos = false;
        bool pos_removed = false;
        float x = 0.0f;

        bool has_vel = false;
        bool vel_removed = false;
    };

    math::FastRandom rng;
    auto rand_index = [&](usize size) { return usize(rng() % size); };

    EntityWorld world;
    core::FlatHashMap<EntityId, State> model;
    core::Vector<EntityId> live;

    const EntityGroupProvider* both = world.get_or_create_group_provider<Position, Velocity>();

    for(usize round = 0; round != 25; ++round) {
        for(usize op = 0; op != 300; ++op) {
            const u32 kind = rng() % 100;
            if(kind < 30 || live.is_empty()) {
                const EntityId id = world.create_entity();
                y_test_assert(model.find(id) == model.end());
                model[id] = State{};
                live << id;
            } else {
                const usize index = rand_index(live.size());
                const EntityId id = live[index];
                State& state = model[id];

                if(kind < 40) {
                    world.remove_entity(id);
                    live.erase_unordered(live.begin() + index);
                } else if(kind < 60) {
                    state.x = float(rng() % 1000);
                    world.add_or_replace_component<Position>(id, state.x, 0.0f);
                    state.has_pos = true;
                    state.pos_removed = false;
                } else if(kind < 72) {
                    world.remove_component<Position>(id);
                    state.pos_removed = state.has_pos;
                } else if(kind < 88) {
                    world.add_or_replace_component<Velocity>(id);
                    state.has_vel = true;
                    state.vel_removed = false;
                } else if(kind < 95) {
                    world.remove_component<Velocity>(id);
                    state.vel_removed = state.has_vel;
                } else {
                    if(Position* pos = world.component_mut<Position>(id)) {
                        y_test_assert(state.has_pos);
                        pos->x += 1.0f;
                        state.x += 1.0f;
                    } else {
                        y_test_assert(!state.has_pos);
                    }
                }
            }
        }

        // Before processing, pending removals are still visible
        for(const auto& [id, state] : model) {
            y_test_assert(world.exists(id));
            const Position* pos = world.component<Position>(id);
            y_test_assert(!!pos == state.has_pos);
            if(pos) {
                y_test_assert(pos->x == state.x);
            }
            y_test_assert(world.has_component<Velocity>(id) == state.has_vel);
        }

        world.process_deferred_changes();

        // Apply the deferred changes to the model
        usize both_count = 0;
        usize pos_count = 0;
        usize vel_count = 0;
        core::Vector<EntityId> dead;
        for(auto&& [id, state] : model) {
            if(!contains(live, id)) {
                y_test_assert(!world.exists(id));
                dead << id;
                continue;
            }

            state.has_pos &= !state.pos_removed;
            state.has_vel &= !state.vel_removed;
            state.pos_removed = state.vel_removed = false;

            pos_count += state.has_pos ? 1 : 0;
            vel_count += state.has_vel ? 1 : 0;
            both_count += (state.has_pos && state.has_vel) ? 1 : 0;
        }
        for(const EntityId id : dead) {
            model.erase(id);
        }

        y_test_assert(world.entity_count() == model.size());
        y_test_assert(world.entity_count() == live.size());
        y_test_assert(world.component_set<Position>().size() == pos_count);
        y_test_assert(world.component_set<Velocity>().size() == vel_count);
        y_test_assert(both->ids().size() == both_count);
        y_test_assert((world.create_group<Position, Velocity>().size() == both_count));

        for(const auto& [id, state] : model) {
            y_test_assert(world.exists(id));
            const Position* pos = world.component<Position>(id);
            y_test_assert(!!pos == state.has_pos);
            if(pos) {
                y_test_assert(pos->x == state.x);
            }
            y_test_assert(world.has_component<Velocity>(id) == state.has_vel);
            y_test_assert(both->ids().contains(id) == (state.has_pos && state.has_vel));
        }
    }

    // Round trip the final state
    EntityWorld loaded;
    save_and_load(world, loaded);
    y_test_assert(loaded.entity_count() == model.size());
    for(const auto& [id, state] : model) {
        y_test_assert(loaded.exists(id));
        const Position* pos = loaded.component<Position>(id);
        y_test_assert(!!pos == state.has_pos);
        if(pos) {
            y_test_assert(pos->x == state.x);
        }
        y_test_assert(loaded.has_component<Velocity>(id) == state.has_vel);
    }
}

y_test_func("EntityWorld version 0 ids") {
    EntityWorld world;

    const EntityId a = world.create_entity_with_id(EntityId(0, 0));
    const EntityId b = world.create_entity_with_id(EntityId(1, 0));
    const EntityId c = world.create_entity_with_id(EntityId(2, 0));
    world.add_or_replace_component<Position>(b, 1.0f, 1.0f);
    world.add_tag(b, "tag");

    y_test_assert(!world.has_component<Position>(a));
    y_test_assert(!world.component<Position>(a));
    y_test_assert(!world.component<Position>(c));
    y_test_assert(world.component<Position>(b)->x == 1.0f);
    y_test_assert(!world.has_tag(a, "tag"));
    y_test_assert(world.has_tag(b, "tag"));

    world.process_deferred_changes();
    y_test_assert(world.create_group<Position>().size() == 1);
    y_test_assert(world.create_group<Position>().ids()[0] == b);

    world.component_mut<Position>(b);
    y_test_assert(!container_of<Position>(world)->mutated_ids().contains(a));

    world.remove_entity(a);
    y_test_assert(!world.pending_deletions().contains(b));
    world.process_deferred_changes();
    y_test_assert(!world.exists(a));
    y_test_assert(world.exists(b));
    y_test_assert(world.component<Position>(b)->x == 1.0f);
}

y_test_func("EntityWorld randomized hierarchy") {
    math::FastRandom rng(1234);

    EntityWorld world;
    core::Vector<EntityId> ids;
    for(usize i = 0; i != 64; ++i) {
        ids << world.create_entity();
    }

    for(usize i = 0; i != 2000; ++i) {
        const EntityId id = ids[rng() % ids.size()];
        const EntityId parent = (rng() % 8) ? ids[rng() % ids.size()] : EntityId();
        if(id == parent || world.is_parent(parent, id)) {
            continue; // Would create a cycle
        }

        world.set_parent(id, parent);
        y_test_assert(world.parent(id) == parent);
    }

    // Every child lists its parent, every parent lists its children exactly once
    for(const EntityId id : ids) {
        for(const EntityId child : world.direct_children(id)) {
            y_test_assert(world.parent(child) == id);
        }
        if(world.has_parent(id)) {
            usize found = 0;
            for(const EntityId child : world.direct_children(world.parent(id))) {
                found += (child == id) ? 1 : 0;
            }
            y_test_assert(found == 1);
            y_test_assert(!world.is_parent(world.parent(id), id));
        }
    }

    // Remove half the entities, the remaining ones must stay consistent
    for(usize i = 0; i != ids.size(); i += 2) {
        world.remove_entity(ids[i]);
    }
    world.process_deferred_changes();

    for(usize i = 1; i < ids.size(); i += 2) {
        const EntityId id = ids[i];
        y_test_assert(world.exists(id));
        if(world.has_parent(id)) {
            y_test_assert(world.exists(world.parent(id)));
        }
        for(const EntityId child : world.direct_children(id)) {
            y_test_assert(world.exists(child));
            y_test_assert(world.parent(child) == id);
        }
    }
}

bool is_blueprint_consistent(const Blueprint& blueprint) {
    const auto nodes = blueprint.all_nodes();
    const auto links = blueprint.links();

    if(!links_are_ordered(blueprint)) {
        return false;
    }

    for(usize i = 0; i != links.size(); ++i) {
        const BlueprintLink& link = links[i];
        if(link.dst_node >= nodes.size() || link.dst_pin >= nodes[link.dst_node]->input_pins().size()) {
            return false;
        }
        if(link.src_pin >= nodes[link.src_node]->output_pins().size()) {
            return false;
        }

        // At most one link per input pin
        for(usize j = i + 1; j != links.size(); ++j) {
            if(links[j].dst_node == link.dst_node && links[j].dst_pin == link.dst_pin) {
                return false;
            }
        }

        // Resolved types match
        const BlueprintParamType* src_type = nodes[link.src_node]->output_pins()[link.src_pin].type;
        const BlueprintParamType* dst_type = nodes[link.dst_node]->input_pins()[link.dst_pin].type;
        if(src_type && dst_type && src_type != dst_type) {
            return false;
        }

        if(!blueprint.downstream_nodes(nodes[link.src_node].get())[link.dst_node]) {
            return false;
        }
    }

    // The only error the API can produce is an unresolved generic type
    if(const auto res = blueprint.validate(); res.is_error()) {
        return res.error().error == "Unresolved generic type";
    }
    return blueprint.create_instance().is_ok();
}

bool same_blueprints(const Blueprint& a, const Blueprint& b) {
    if(a.all_nodes().size() != b.all_nodes().size() || a.links().size() != b.links().size()) {
        return false;
    }
    for(usize i = 0; i != a.all_nodes().size(); ++i) {
        const BlueprintNode* na = a.all_nodes()[i].get();
        const BlueprintNode* nb = b.all_nodes()[i].get();
        if(na->name() != nb->name() || na->generic_type() != nb->generic_type()) {
            return false;
        }
    }
    for(usize i = 0; i != a.links().size(); ++i) {
        const BlueprintLink& la = a.links()[i];
        const BlueprintLink& lb = b.links()[i];
        if(la.src_node != lb.src_node || la.src_pin != lb.src_pin || la.dst_node != lb.dst_node || la.dst_pin != lb.dst_pin) {
            return false;
        }
    }
    return true;
}

y_test_func("Blueprint randomized graph edits") {
    core::Vector<std::unique_ptr<BlueprintNodeFactory>> factories;
    add_all_nodes(factories);

    math::FastRandom rng;
    auto rand_index = [&](usize size) { return usize(rng() % size); };

    Blueprint blueprint;
    for(usize op = 0; op != 3000; ++op) {
        const auto nodes = blueprint.all_nodes();
        const u32 kind = rng() % 100;

        if(nodes.size() < 2 || (kind < 20 && nodes.size() < 30)) {
            blueprint.add_node(factories[rand_index(factories.size())]->create_node());
        } else if(kind < 30) {
            blueprint.remove_node(nodes[rand_index(nodes.size())].get());
        } else if(kind < 85) {
            // Most random links are invalid, try a few times
            for(usize attempt = 0; attempt != 20; ++attempt) {
                const BlueprintNode* src = nodes[rand_index(nodes.size())].get();
                const BlueprintNode* dst = nodes[rand_index(nodes.size())].get();
                if(src->output_pins().is_empty() || dst->input_pins().is_empty()) {
                    continue;
                }
                const usize src_pin = rand_index(src->output_pins().size());
                const usize dst_pin = rand_index(dst->input_pins().size());
                if(blueprint.is_link_valid(src, src_pin, dst, dst_pin)) {
                    blueprint.add_link(src, src_pin, dst, dst_pin);
                    break;
                }
            }
        } else if(!blueprint.links().is_empty()) {
            const BlueprintLink link = blueprint.links()[rand_index(blueprint.links().size())];
            blueprint.remove_link(nodes[link.dst_node].get(), link.dst_pin);
        }

        y_test_assert(is_blueprint_consistent(blueprint));

        if(op % 50 == 0) {
            io2::Buffer buffer;
            {
                serde3::WritableArchive arc(buffer);
                y_test_assert(arc.serialize(blueprint).is_ok());
            }
            buffer.reset();

            Blueprint loaded;
            y_test_assert(serde3::ReadableArchive(buffer).deserialize(loaded).is_ok());
            y_test_assert(same_blueprints(blueprint, loaded));

            // Running every entry point with a zeroed payload may fail, but must not crash
            if(auto instance = loaded.create_instance(); instance.is_ok()) {
                alignas(16) std::array<u8, 64> payload = {};
                for(const BlueprintInstance::EntryPoint& entry : instance.unwrap().entry_points()) {
                    y_test_assert(entry.payload_size <= payload.size());
                    unused(instance.unwrap().trigger(entry, payload.data()));
                }
            }
        }
    }
}

}



using namespace y;

int main() {
    const bool ok = test::run_tests();

    if(ok) {
        log_msg("All tests OK\n");
    } else {
        log_msg("Tests failed\n", Log::Error);
    }

    return ok ? 0 : 1;
}

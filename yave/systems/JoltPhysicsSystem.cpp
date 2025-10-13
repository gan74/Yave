/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "JoltPhysicsSystem.h"

#include <yave/components/ColliderComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/meshes/MeshData.h>
#include <yave/ecs/EntityWorld.h>
#include <yave/systems/TimeSystem.h>


// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Renderer/DebugRenderer.h>
#include <Jolt/Renderer/DebugRendererSimple.h>

#include <yave/utils/DirectDraw.h>

#include <y/utils/log.h>


#include <thread>


namespace yave {

y_force_inline static math::Vec4 to_y(const JPH::Vec4& v) {
    return math::Vec4(v.GetX(), v.GetY(), v.GetZ(), v.GetW());
}

y_force_inline static math::Vec3 to_y(const JPH::Vec3& v) {
    return math::Vec3(v.GetX(), v.GetY(), v.GetZ());
}

y_force_inline static math::Vec3 to_y(const JPH::Float3& v) {
    return math::Vec3(v.x, v.y, v.z);
}

y_force_inline static math::Quaternion<> to_y(const JPH::Quat& q) {
    return math::Quaternion<>(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
}

y_force_inline static JPH::Vec3 to_jph(const math::Vec3& v) {
    return JPH::Vec3(v.x(), v.y(), v.z());
}

y_force_inline static JPH::Float3 to_jph_float3(const math::Vec3& v) {
    return JPH::Float3(v.x(), v.y(), v.z());
}

y_force_inline static JPH::Quat to_jph(const math::Quaternion<>& q) {
    return JPH::Quat(q.x(), q.y(), q.z(), q.w());
}

y_force_inline static JPH::EMotionType to_jph(ColliderComponent::Type type) {
    return JPH::EMotionType(type);
}

y_force_inline static JPH::IndexedTriangle to_jph(IndexedTriangle t) {
    return JPH::IndexedTriangle(t[0], t[1], t[2], 0);
}



class DebugDrawer final : public JPH::DebugRendererSimple {
    public:
        DebugDrawer() : _prim(editor::debug_drawer().add_primitive("Jolt Debug")) {
            JPH::DebugRenderer::Initialize();
        }

        void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override {
            _prim->add_line(color.mU32, to_y(from), to_y(to));
        }

        void DrawTriangle(JPH::RVec3Arg v0, JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::ColorArg color, ECastShadow cast_shadow) override {
            unused(cast_shadow);
            _prim->add_triangle((color.mU32 & 0x00FFFFFF) | 0x80000000, to_y(v0), to_y(v1), to_y(v2));
        }

        void DrawText3D(JPH::RVec3Arg pos, const std::string_view& str, JPH::ColorArg color, float height) override {
            unused(pos, str, color, height);
            log_msg("DrawText3D is not supported");
        }

    private:
        DirectDrawPrimitive* _prim = nullptr;
};

struct BPLayerInterface final : JPH::BroadPhaseLayerInterface {
    BPLayerInterface()  = default;

    u32 GetNumBroadPhaseLayers() const override {
        return 1;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        unused(layer);
        y_debug_assert(!layer);
        return JPH::BroadPhaseLayer(0);
    }
};

struct ObjectVsPBLayerFilter : JPH::ObjectVsBroadPhaseLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override {
        return true;
    }
};

struct ObjectLayerPairFilter : JPH::ObjectLayerPairFilter {
    bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override {
        return true;
    }
};

struct JoltActiveComponent {
    JPH::BodyID id;

    y_no_serde3()
};

struct BodyActivationListener : JPH::BodyActivationListener, NonMovable {
        BodyActivationListener(ecs::EntityWorld* w) : world(w) {
    }

    void OnBodyActivated(const JPH::BodyID& body_id, u64 body_user_data) override {
        unused(body_id);
        if(const ecs::EntityId id = ecs::EntityId::from_u64(body_user_data); id.is_valid()) {
            // log_msg(fmt("body activated: {}", body_id.GetIndexAndSequenceNumber()));
            world->add_or_replace_component<JoltActiveComponent>(id, body_id);
        }
    }

    void OnBodyDeactivated(const JPH::BodyID& body_id, u64 body_user_data) override {
        unused(body_id);
        if(const ecs::EntityId id = ecs::EntityId::from_u64(body_user_data); id.is_valid()) {
            // log_msg(fmt("body deactivated: {}", body_id.GetIndexAndSequenceNumber()));
            world->remove_component<JoltActiveComponent>(id);
        }
    }

    ecs::EntityWorld* world = nullptr;

};

struct JoltData : NonMovable {
    BodyActivationListener activation_listener;
    JPH::TempAllocatorImpl temp_allocator;
    JPH::JobSystemSingleThreaded job_system;
    BPLayerInterface bp_layer_interface;
    ObjectVsPBLayerFilter obj_vs_bp_layer_filter;
    ObjectLayerPairFilter obj_vs_obj_layer_filter;
    JPH::BodyInterface* body_interface = nullptr;

    JPH::PhysicsSystem physics_system;

    core::FlatHashMap<const StaticMesh*, JPH::Ref<JPH::Shape>> static_shapes;
    core::FlatHashMap<const StaticMesh*, JPH::Ref<JPH::Shape>> movable_shapes;


    JoltData(ecs::EntityWorld* world) : activation_listener(world), temp_allocator(1024 * 1024 * 8), job_system(256) {

        physics_system.Init(16 * 1024, 0, 1024, 1024, bp_layer_interface, obj_vs_bp_layer_filter, obj_vs_obj_layer_filter);
        physics_system.SetBodyActivationListener(&activation_listener);
        body_interface = &physics_system.GetBodyInterface();

        physics_system.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));
    }


    void update(float dt) {
        physics_system.Update(dt, 1, &temp_allocator, &job_system);
    }
};







static void init_jolt() {
    static bool init = false;
    if(init) {
        return;
    }

    init = true;

    JPH::Trace = [](const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char buffer[512] = {};
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        log_msg(buffer, Log::Warning);
    };

    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}

static JPH::Ref<JPH::Shape> create_static_shape(JoltData* data, const StaticMesh* mesh) {
    auto& shape = data->static_shapes[mesh];

    if(!shape.GetPtr()) {
        y_profile();

        const MeshTriangleData& triangle_data = mesh->triangle_data();
        JPH::Array<JPH::Float3> vertices(triangle_data.positions.size());
        JPH::Array<JPH::IndexedTriangle> triangles(triangle_data.triangles.size());
        std::transform(triangle_data.positions.begin(), triangle_data.positions.end(), vertices.begin(), [&](math::Vec3 p) { return to_jph_float3(p); });
        std::transform(triangle_data.triangles.begin(), triangle_data.triangles.end(), triangles.begin(), [](IndexedTriangle t) { return to_jph(t); });

        const JPH::MeshShapeSettings shape_settings(vertices, triangles);
        shape = shape_settings.Create().Get();
    }

    y_debug_assert(shape.GetPtr());
    return shape;
}

static JPH::Ref<JPH::Shape> create_movable_shape(JoltData* data, const StaticMesh* mesh) {
    auto& shape = data->movable_shapes[mesh];

    if(!shape.GetPtr()) {
        y_profile();

        const MeshTriangleData& triangle_data = mesh->triangle_data();
        JPH::Array<JPH::Vec3> vertices(triangle_data.positions.size());
        std::transform(triangle_data.positions.begin(), triangle_data.positions.end(), vertices.begin(), [&](math::Vec3 p) { return to_jph(p); });

        const JPH::ConvexHullShapeSettings shape_settings(vertices.data(), int(vertices.size()));
        shape = shape_settings.Create().Get();
    }

    y_debug_assert(shape.GetPtr());
    return shape;
}






JoltPhysicsSystem::JoltPhysicsSystem() : ecs::System("JoltPhysicsSystem") {
    init_jolt();
}

JoltPhysicsSystem::~JoltPhysicsSystem() {

}

void JoltPhysicsSystem::set_debug_draw(bool enable) {
    _debug_draw = enable;
}

void JoltPhysicsSystem::setup(ecs::SystemScheduler& sched) {
    _jolt = std::make_unique<JoltData>(&world());

    const auto collect_job = sched.schedule(ecs::SystemSchedule::Update, "Collect colliders", [this](ecs::EntityGroup<
            ecs::AnyChanged<ColliderComponent>,
            ecs::AnyChanged<StaticMeshComponent>,
            TransformableComponent
        >&& group) {

        for(auto&& [id, coll, mesh, tr] : group.id_components()) {
            if(!mesh.is_fully_loaded()) {
                continue;
            }

            if(coll._body_id != ColliderComponent::invalid_index) {
                _jolt->body_interface->RemoveBody(JPH::BodyID(coll._body_id));
                coll._body_id = ColliderComponent::invalid_index;
            }

            if(const AssetPtr<StaticMesh>& static_mesh = mesh.mesh()) {
                const math::Transform<>& transform = tr.transform();
                const auto [translation, rotation, scale] = transform.decompose();

                JPH::Ref<JPH::Shape> shape = coll._type == ColliderComponent::Type::Static
                    ? create_static_shape(_jolt.get(), static_mesh.get())
                    : create_movable_shape(_jolt.get(), static_mesh.get())
                ;

                {
                    JPH::ScaledShapeSettings scaled_settings(shape, to_jph(scale));
                    shape = scaled_settings.Create().Get();
                }

                JPH::BodyCreationSettings body_settings = JPH::BodyCreationSettings(shape, to_jph(translation), to_jph(rotation), to_jph(coll._type), JPH::ObjectLayer(0));
                body_settings.mUserData = id.as_u64();

                coll._scale = scale;
                coll._body_id = _jolt->body_interface->CreateAndAddBody(body_settings, JPH::EActivation::Activate).GetIndexAndSequenceNumber();
            }
        }
    });



    const auto physics_job = sched.schedule(ecs::SystemSchedule::Update, "Update physics", [this](const ecs::EntityWorld& world) {
        const float dt = TimeSystem::dt(world);
        if(dt > 0.0f) {
            _jolt->update(std::min(dt, 0.2f));
        }
    }, collect_job);

    sched.schedule(ecs::SystemSchedule::Update, "Copy transforms", [this](ecs::EntityGroup<
            JoltActiveComponent,
            ColliderComponent,
            ecs::Mutate<TransformableComponent>
        >&& group) {

        const JPH::BodyLockInterface& lock_interface = _jolt->physics_system.GetBodyLockInterface();
        for(auto&& [id, active, coll, tr] : group.id_components()) {
            y_debug_assert(!active.id.IsInvalid());

            const JPH::BodyLockRead lock(lock_interface, active.id);
            if(!lock.Succeeded()) {
                continue;
            }

            const JPH::Body& body = lock.GetBody();
            y_debug_assert(ecs::EntityId::from_u64(body.GetUserData()) == id);

            tr.set_transform(math::Transform<>(to_y(body.GetPosition()), to_y(body.GetRotation()), coll._scale));
        }
    }, physics_job);

    sched.schedule(ecs::SystemSchedule::PostUpdate, "Debug draw", [this]() {
        if(!_debug_draw) {
            return;
        }
        DebugDrawer renderer;
        _jolt->physics_system.DrawBodies(JPH::BodyManager::DrawSettings{}, &renderer);
    });
}


}


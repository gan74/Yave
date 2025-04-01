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
#include <yave/ecs/EntityWorld.h>


// Jolt includes
#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
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

y_force_inline static JPH::Quat to_jph(const math::Quaternion<>& q) {
    return JPH::Quat(q.x(), q.y(), q.z(), q.w());
}

y_force_inline static JPH::EMotionType to_jph(ColliderComponent::Type type) {
    return JPH::EMotionType(type);
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
            _prim->add_triangle((color.mU32 & 0x00FFFFFF) | 0xF0000000, to_y(v0), to_y(v1), to_y(v2));
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
};

struct BodyActivationListener : JPH::BodyActivationListener, NonMovable {
        BodyActivationListener(ecs::EntityWorld* w) : world(w) {
    }

    void OnBodyActivated(const JPH::BodyID& body_id, u64 body_user_data) override {
        if(const ecs::EntityId id = ecs::EntityId::from_u64(body_user_data); id.is_valid()) {
            world->add_or_replace_component<JoltActiveComponent>(id, body_id);
        }
    }

    void OnBodyDeactivated(const JPH::BodyID&, u64 body_user_data) override {
        if(const ecs::EntityId id = ecs::EntityId::from_u64(body_user_data); id.is_valid()) {
            world->remove_component<JoltActiveComponent>(id);
        }
    }

    ecs::EntityWorld* world = nullptr;

};

struct JoltData : NonMovable {
    BodyActivationListener activation_listener;
    JPH::TempAllocatorImpl temp_allocator;
    JPH::JobSystemThreadPool thread_pool;
    BPLayerInterface bp_layer_interface;
    ObjectVsPBLayerFilter obj_vs_bp_layer_filter;
    ObjectLayerPairFilter obj_vs_obj_layer_filter;
    JPH::BodyInterface* body_interface = nullptr;

    JPH::PhysicsSystem physics_system;


    JoltData(ecs::EntityWorld* world) : activation_listener(world), temp_allocator(1024 * 1024 * 8), thread_pool(2048, 8, std::thread::hardware_concurrency() - 1) {

        physics_system.Init(1024, 0, 1024, 1024, bp_layer_interface, obj_vs_bp_layer_filter, obj_vs_obj_layer_filter);
        physics_system.SetBodyActivationListener(&activation_listener);
        body_interface = &physics_system.GetBodyInterface();

        physics_system.SetGravity(JPH::Vec3(0.0f, 0.0f, -9.81f));

        // Next we can create a rigid body to serve as the floor, we make a large box
        // Create the settings for the collision volume (the shape).
        // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
        JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(100.0f, 100.0f, 10.0f));
        // floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

        // Create the shape
        JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
        JPH::ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

        // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
        JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0f, 0.0f, -10.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, JPH::ObjectLayer(0));
        floor_settings.mUserData = ecs::EntityId().as_u64();

        // Create the actual rigid body
        JPH::Body *floor = body_interface->CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

        // Add it to the world
        body_interface->AddBody(floor->GetID(), JPH::EActivation::DontActivate);

        // Now create a dynamic body to bounce on the floor
        // Note that this uses the shorthand version of creating and adding a body to the world
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0f, 0.0f, 2.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, JPH::ObjectLayer(0));
        sphere_settings.mUserData = ecs::EntityId().as_u64();
        sphere_settings.mRestitution = 1.0f;

        JPH::BodyID sphere_id = body_interface->CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

        // Now you can interact with the dynamic body, in this case we're going to give it a velocity.
        // (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
        body_interface->SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, 0.0f, -5.0f));


        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        physics_system.OptimizeBroadPhase();
    }


    void update(float dt) {
        physics_system.Update(dt, 1, &temp_allocator, &thread_pool);
    }
};







static void init_jolt() {
    static bool init = false;
    if(init) {
        return;
    }

    init = true;

    JPH::RegisterDefaultAllocator();

    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
}



JoltPhysicsSystem::JoltPhysicsSystem() : ecs::System("JoltPhysicsSystem") {
    init_jolt();
}

JoltPhysicsSystem::~JoltPhysicsSystem() {

}

void JoltPhysicsSystem::setup(ecs::SystemScheduler& sched) {
    _jolt = std::make_unique<JoltData>(&world());

    sched.schedule(ecs::SystemSchedule::Tick, "Collect colliders", [this](ecs::EntityGroup<
            ecs::AnyChanged<ColliderComponent>,
            ecs::AnyChanged<StaticMeshComponent>,
            TransformableComponent
        >&& group) {

        for(auto&& [id, coll, mesh, tr] : group.id_components()) {
            if(!mesh.is_fully_loaded()) {
                continue;
            }

            if(!coll._body_id.IsInvalid()) {
                _jolt->body_interface->RemoveBody(coll._body_id);
                coll._body_id = {};
            }

            if(const AssetPtr<StaticMesh>& static_mesh = mesh.mesh()) {
                const AABB aabb = static_mesh->aabb();
                const math::Transform<>& transform = tr.transform();

                const auto [translation, rotation, scale] = transform.decompose();
                const math::Vec3 center = transform.transform_point(aabb.center());

                const JPH::BoxShapeSettings shape_settings(to_jph(static_mesh->aabb().half_extent() * scale));
                const JPH::ShapeSettings::ShapeResult shape_result = shape_settings.Create();

                JPH::BodyCreationSettings body_settings(shape_result.Get(), to_jph(center), to_jph(rotation), to_jph(coll._type), JPH::ObjectLayer(0));
                body_settings.mUserData = id.as_u64();

                coll._scale = scale;
                coll._body_id = _jolt->body_interface->CreateAndAddBody(body_settings, JPH::EActivation::Activate);

                log_msg(fmt("body created: {}", coll._body_id.GetIndexAndSequenceNumber()));
            }
        }
    });

    sched.schedule(ecs::SystemSchedule::Update, "Update physics", [this]() {
        const double dt = _time.reset().to_secs();
        _jolt->update(float(std::min(dt, 0.1)));
    });

    sched.schedule(ecs::SystemSchedule::PostUpdate, "Copy transforms", [this](ecs::EntityGroup<
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
            tr.set_transform(math::Transform<>(to_y(body.GetPosition()), to_y(body.GetRotation()), coll._scale));
        }
    });

    sched.schedule(ecs::SystemSchedule::PostUpdate, "Debug draw", [this]() {
        DebugDrawer renderer;
        _jolt->physics_system.DrawBodies(JPH::BodyManager::DrawSettings{}, &renderer);
    });
}


}


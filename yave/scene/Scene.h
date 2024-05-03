/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_SCENE_SCENE_H
#define YAVE_SCENE_SCENE_H

#include "TransformManager.h"

#include <yave/ecs/ecs.h>

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>

#include <functional>

namespace yave {

enum class PassType {
    Depth,
    GBuffer,
    Id,
};

struct SceneObject {
    ecs::EntityId id;
};

struct TransformableSceneObject : SceneObject {
    u32 transform_index = u32(-1);
    AABB global_aabb;
};

using StaticMeshObject = std::tuple<TransformableSceneObject, StaticMeshComponent>;
using PointLightObject = std::tuple<TransformableSceneObject, PointLightComponent>;
using SpotLightObject = std::tuple<TransformableSceneObject, SpotLightComponent>;
using DirectionalLightObject = std::tuple<SceneObject, DirectionalLightComponent>;
using SkyLightObject = std::tuple<SceneObject, SkyLightComponent>;

class Scene : NonMovable {
    static constexpr BufferUsage buffer_usage = BufferUsage::StorageBit | BufferUsage::TransferDstBit | BufferUsage::TransferSrcBit;
    using TransformBuffer = TypedBuffer<shader::TransformableData, buffer_usage>;

    public:
        using RenderFunc = std::function<void(RenderPassRecorder& render_pass, const FrameGraphPass* pass)>;


        Scene();

        virtual ~Scene();


        const math::Transform<>& transform(const TransformableSceneObject& obj) const;

        core::Vector<const StaticMeshObject*> gather_visible_meshes(const Camera& cam) const;


        RenderFunc prepare_render(FrameGraphPassBuilder& builder, const Camera& cam,  PassType pass_type) const;



        core::Span<StaticMeshObject>        meshes() const          { return _meshes; }
        core::Span<PointLightObject>        point_lights() const    { return _point_lights; }
        core::Span<SpotLightObject>         spot_lights() const     { return _spot_lights; }
        core::Span<DirectionalLightObject>  directionals() const    { return _directionals; }
        core::Span<SkyLightObject>          sky_lights() const      { return _sky_lights; }

    protected:
        core::Vector<StaticMeshObject> _meshes;
        core::Vector<PointLightObject> _point_lights;
        core::Vector<SpotLightObject> _spot_lights;

        core::Vector<DirectionalLightObject> _directionals;
        core::Vector<SkyLightObject> _sky_lights;

        TransformManager _transform_manager;
};

}

#endif // YAVE_SCENE_SCENE_H


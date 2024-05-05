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

#include <yave/components/StaticMeshComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>

#include <yave/camera/Camera.h>

#include <functional>

namespace yave {

enum class PassType {
    Depth,
    GBuffer,
    Id,
};



template<typename T>
struct SceneObject {
    T component;
    u32 entity_index = u32(-1);
};

struct TransformableSceneObjectData {
    u32 transform_index = u32(-1);
    AABB global_aabb;
};

template<typename T>
struct TransformableSceneObject : TransformableSceneObjectData, SceneObject<T> {
};



using StaticMeshObject          = TransformableSceneObject<StaticMeshComponent>;
using PointLightObject          = TransformableSceneObject<PointLightComponent>;
using SpotLightObject           = TransformableSceneObject<SpotLightComponent>;
using DirectionalLightObject    = SceneObject<DirectionalLightComponent>;
using SkyLightObject            = SceneObject<SkyLightComponent>;

class Scene : NonMovable {
    public:
        using RenderFunc = std::function<void(RenderPassRecorder& render_pass, const FrameGraphPass* pass)>;


        Scene();

        virtual ~Scene();


        const math::Transform<>& transform(const TransformableSceneObjectData& obj) const;


        RenderFunc prepare_render(FrameGraphPassBuilder& builder, const SceneVisibility& visibility, PassType pass_type) const;



        core::Span<StaticMeshObject>        meshes() const          { return _meshes; }
        core::Span<PointLightObject>        point_lights() const    { return _point_lights; }
        core::Span<SpotLightObject>         spot_lights() const     { return _spot_lights; }
        core::Span<DirectionalLightObject>  directionals() const    { return _directionals; }
        core::Span<SkyLightObject>          sky_lights() const      { return _sky_lights; }


        template<typename T>
        static auto gather_visible(core::Span<TransformableSceneObject<T>> objects, const Camera& cam) {
            y_profile();

            const Frustum frustum = cam.frustum();

            core::Vector<const TransformableSceneObject<T>*> visible;
            for(const auto& obj : objects) {
                if(frustum.intersection(obj.global_aabb) != Intersection::Outside) {
                    visible << &obj;
                }
            }

            return visible;
        }

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


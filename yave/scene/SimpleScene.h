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
#ifndef YAVE_SCENE_SIMPLESCENE_H
#define YAVE_SCENE_SIMPLESCENE_H

#include "Scene.h"

#include <yave/components/TransformableComponent.h>

namespace yave {

class SimpleScene : public Scene {
    public:
        StaticMeshObject& add(StaticMeshComponent mesh, const math::Transform<>& transform = {});
        PointLightObject& add(PointLightComponent light, const math::Transform<>& transform = {});
        SpotLightObject& add(SpotLightComponent light, const math::Transform<>& transform = {});
        DirectionalLightObject& add(DirectionalLightComponent light);
        SkyLightObject& add(SkyLightComponent light);

        AtmosphereObject& set_atmosphere(AtmosphereComponent atmosphere, DirectionalLightComponent sun = {});

        template<typename T>
        void set_transform(TransformableSceneObject<T>& obj, const math::Transform<>& transform) {
            y_debug_assert(obj.has_transform());
            _transform_manager.set_transform(obj.transform_index, transform);
            obj.global_aabb = TransformableComponent(transform).to_global(obj.component.aabb());
        }

        void clear();
        void update_transforms();

    private:
        template<typename T>
        TransformableSceneObject<T>& add_transformable(core::Vector<TransformableSceneObject<T>>& storage, T component, const math::Transform<>& transform);

        template<typename T>
        SceneObject<T>& add_object(core::Vector<SceneObject<T>>& storage, T component);

        template<typename T>
        void free_transforms(core::Vector<TransformableSceneObject<T>>& storage);
};

}

#endif // YAVE_SCENE_SIMPLESCENE_H

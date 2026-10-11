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

#include "SimpleScene.h"

#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/graphics.h>

namespace yave {


template<typename T>
TransformableSceneObject<T>& SimpleScene::add_transformable(core::Vector<TransformableSceneObject<T>>& storage, T component, const math::Transform<>& transform) {
    auto& obj = storage.emplace_back();
    obj.component = std::move(component);
    obj.transform_index = _transform_manager.alloc_transform();
    set_transform(obj, transform);
    return obj;
}

template<typename T>
SceneObject<T>& SimpleScene::add_object(core::Vector<SceneObject<T>>& storage, T component) {
    auto& obj = storage.emplace_back();
    obj.component = std::move(component);
    return obj;
}

template<typename T>
void SimpleScene::free_transforms(core::Vector<TransformableSceneObject<T>>& storage) {
    for(const auto& obj : storage) {
        if(obj.has_transform()) {
            _transform_manager.free_transform(obj.transform_index);
        }
    }
    storage.make_empty();
}



StaticMeshObject& SimpleScene::add(StaticMeshComponent mesh, const math::Transform<>& transform) {
    return add_transformable(_meshes, std::move(mesh), transform);
}

PointLightObject& SimpleScene::add(PointLightComponent light, const math::Transform<>& transform) {
    return add_transformable(_point_lights, std::move(light), transform);
}

SpotLightObject& SimpleScene::add(SpotLightComponent light, const math::Transform<>& transform) {
    return add_transformable(_spot_lights, std::move(light), transform);
}

DirectionalLightObject& SimpleScene::add(DirectionalLightComponent light) {
    return add_object(_directionals, std::move(light));
}

SkyLightObject& SimpleScene::add(SkyLightComponent light) {
    return add_object(_sky_lights, std::move(light));
}

AtmosphereObject& SimpleScene::set_atmosphere(AtmosphereComponent atmosphere, DirectionalLightComponent sun) {
    if(!_atmosphere) {
        _atmosphere = std::make_unique<AtmosphereObject>();
    }
    _atmosphere->component = std::move(atmosphere);
    _atmosphere->sun = std::move(sun);
    return *_atmosphere;
}

void SimpleScene::clear() {
    free_transforms(_meshes);
    free_transforms(_point_lights);
    free_transforms(_spot_lights);

    _directionals.make_empty();
    _sky_lights.make_empty();
    _atmosphere = nullptr;
}

void SimpleScene::update_transforms() {
    y_profile();

    if(_transform_manager.need_update()) {
        ComputeCmdBufferRecorder recorder = create_disposable_compute_cmd_buffer();
        _transform_manager.update_buffer(recorder);
        recorder.submit_async();
    }

    update_tlas();
}

}

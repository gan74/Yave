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

#include "SceneView.h"

#include <yave/ecs/EntityWorld.h>

#include <yave/components/TransformableComponent.h>
#include <yave/systems/TransformableManagerSystem.h>

namespace yave {

SceneView::SceneView(const ecs::EntityWorld* world, Camera cam) :
        _world(world),
        _camera(cam) {
}

const ecs::EntityWorld& SceneView::world() const {
    y_debug_assert(has_world());
    return *_world;
}

bool SceneView::has_world() const {
    return _world;
}

const Camera& SceneView::camera() const {
    return _camera;
}

Camera& SceneView::camera() {
    return _camera;
}


core::Vector<ecs::EntityId> SceneView::visible_entities() const {
    y_profile();

    y_debug_assert(has_world());

    const std::array tags = {ecs::tags::not_hidden};

    if(const TransformableManagerSystem* system = _world->find_system<TransformableManagerSystem>()) {
        const core::Vector<ecs::EntityId> visible = system->find_visible(_camera.frustum(), _camera.far_plane_dist());
        return _world->query(visible, tags).to_ids();
    }

    return _world->query<TransformableComponent>(tags).to_ids();
}

}


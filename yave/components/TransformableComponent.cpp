/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "TransformableComponent.h"

#include <yave/systems/TransformManagerSystem.h>

namespace yave {

TransformableComponent::TransformableComponent(const math::Transform<>& local_transform) : _local_transform(local_transform) {
}

TransformableComponent::TransformableComponent(TransformableComponent&& other) {
    swap(other);
}

TransformableComponent& TransformableComponent::operator=(TransformableComponent&& other) {
    swap(other);
    return *this;
}

TransformableComponent::TransformableComponent(const TransformableComponent& other) {
    set_local_transform(other.local_transform());
}

TransformableComponent& TransformableComponent::operator=(const TransformableComponent& other) {
    set_world_transform(other.world_transform());
    return *this;
}

void TransformableComponent::swap(TransformableComponent& other) {
    std::swap(_local_transform, other._local_transform);
    std::swap(_aabb, other._aabb);
    std::swap(_manager_index, other._manager_index);
    std::swap(_manager, other._manager);
}






void TransformableComponent::set_local_transform(const math::Transform<>& tr) {
    _local_transform = tr;
    if(_manager) {
        _manager->update_world_transform(_manager_index, this);
    }
}

const math::Transform<>& TransformableComponent::local_transform() const {
    return _local_transform;
}




void TransformableComponent::set_world_transform(const math::Transform<>& tr) {
    y_debug_assert(_manager);
    _manager->set_world_transform(*this, tr);
}

const math::Transform<>& TransformableComponent::world_transform() const {
    y_debug_assert(_manager);
    return _manager->world_transform(*this);
}



AABB TransformableComponent::to_world(const AABB& aabb) const {
    // https://zeux.io/2010/10/17/aabb-from-obb-with-component-wise-abs/
    const math::Transform<> world = world_transform();
    const math::Transform<> abs_tr = world.abs();
    const math::Vec3 center = world.transform_point(aabb.center());
    const math::Vec3 half_extent = abs_tr.transform_direction(aabb.half_extent());
    return AABB(center - half_extent, center + half_extent);
}

void TransformableComponent::set_aabb(const AABB& aabb) {
    _aabb = aabb;
}

const AABB& TransformableComponent::local_aabb() const {
    return _aabb;
}

AABB TransformableComponent::world_aabb() const {
    return to_world(_aabb);
}

void TransformableComponent::inspect(ecs::ComponentInspector* inspector) {
    math::Transform<> local_transform = _local_transform;
    inspector->inspect("Transform", local_transform);

    if(!std::equal(local_transform.matrix().begin(), local_transform.matrix().end(), _local_transform.matrix().begin())) {
        set_local_transform(_local_transform);
    }
}


}


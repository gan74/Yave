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

#include <yave/scene/OctreeNode.h>

namespace yave {

TransformableComponent::TransformableComponent(const math::Transform<>& world_transform) : _world_transform(world_transform) {
}

TransformableComponent::TransformableComponent(TransformableComponent&& other) {
    swap(other);
}

TransformableComponent& TransformableComponent::operator=(TransformableComponent&& other) {
    swap(other);
    return *this;
}

TransformableComponent::TransformableComponent(const TransformableComponent& other) {
    set_world_transform(other.world_transform());
}

TransformableComponent& TransformableComponent::operator=(const TransformableComponent& other) {
    set_world_transform(other.world_transform());
    return *this;
}

void TransformableComponent::swap(TransformableComponent& other) {
    std::swap(_world_transform, other._world_transform);
    std::swap(_aabb, other._aabb);
    std::swap(_node, other._node);
}

void TransformableComponent::set_world_transform(const math::Transform<>& tr) {
    _world_transform = tr;
}

const math::Transform<>& TransformableComponent::world_transform() const {
    return _world_transform;
}

AABB TransformableComponent::to_world(const AABB& aabb) const {
    // https://zeux.io/2010/10/17/aabb-from-obb-with-component-wise-abs/
    const math::Transform<> abs_tr = _world_transform.abs();
    const math::Vec3 center = _world_transform.transform_point(aabb.center());
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

const OctreeNode* TransformableComponent::octree_node() const {
    return _node;
}

void TransformableComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Transform", _world_transform);
}

}


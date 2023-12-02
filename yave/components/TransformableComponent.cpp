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

TransformableComponent::TransformableComponent(const math::Transform<>& transform) : _transform(transform) {
}

TransformableComponent::TransformableComponent(TransformableComponent&& other) {
    swap(other);
}

TransformableComponent& TransformableComponent::operator=(TransformableComponent&& other) {
    swap(other);
    return *this;
}

TransformableComponent::TransformableComponent(const TransformableComponent& other) {
    set_transform(other.transform());
}

TransformableComponent& TransformableComponent::operator=(const TransformableComponent& other) {
    set_transform(other.transform());
    return *this;
}

void TransformableComponent::swap(TransformableComponent& other) {
    std::swap(_transform, other._transform);
    std::swap(_aabb, other._aabb);
    std::swap(_node, other._node);
    std::swap(_transform_index, other._transform_index);
}

void TransformableComponent::set_transform(const math::Transform<>& tr) {
    _transform = tr;
}

void TransformableComponent::set_position(const math::Vec3& pos) {
    _transform.position() = pos;
}

const math::Transform<>& TransformableComponent::transform() const {
    return _transform;
}

const math::Vec3& TransformableComponent::forward() const {
    return _transform.forward();
}

const math::Vec3& TransformableComponent::right() const {
    return _transform.right();
}

const math::Vec3& TransformableComponent::up() const {
    return _transform.up();
}

const math::Vec3& TransformableComponent::position() const {
    return _transform.position();
}

math::Vec3 TransformableComponent::to_global(const math::Vec3& pos) const {
    return _transform.transform_point(pos);
}

AABB TransformableComponent::to_global(const AABB& aabb) const {
    // https://zeux.io/2010/10/17/aabb-from-obb-with-component-wise-abs/
    const math::Transform<> abs_tr = _transform.abs();
    const math::Vec3 center = transform().transform_point(aabb.center());
    const math::Vec3 half_extent = abs_tr.transform_direction(aabb.half_extent());

    return AABB(center - half_extent, center + half_extent);
}

void TransformableComponent::set_aabb(const AABB& aabb) {
    _aabb = aabb;
}

const AABB& TransformableComponent::local_aabb() const {
    return _aabb;
}

AABB TransformableComponent::global_aabb() const {
    return to_global(_aabb);
}

const OctreeNode* TransformableComponent::octree_node() const {
    return _node;
}

u32 TransformableComponent::transform_index() const {
    return _transform_index;
}

void TransformableComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Transform", _transform);
}

}


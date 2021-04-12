/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

void TransformableComponent::set_transform(const math::Transform<>& tr) {
    _transform = tr;
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

math::Vec3& TransformableComponent::position() {
    return _transform.position();
}

math::Vec3 TransformableComponent::to_global(const math::Vec3& pos) const {
    return _transform.to_global(pos);
}

AABB TransformableComponent::to_global(const AABB& aabb) const {
    // https://zeux.io/2010/10/17/aabb-from-obb-with-component-wise-abs/
    const math::Transform<> abs_tr = _transform.abs();
    const math::Vec3 center = transform().to_global(aabb.center());
    const math::Vec3 half_extent = abs_tr.to_global_normal(aabb.half_extent());

    return AABB(center - half_extent, center + half_extent);
}

void TransformableComponent::update_node() {
    /*if(_node) {
        _node->set_dirty(_node_id);
    }*/
}

}


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

#include "SpotLightComponent.h"

#include <yave/camera/Camera.h>
#include <yave/ecs/ComponentInspector.h>

namespace yave {

SpotLightComponent::EnclosingSphere SpotLightComponent::enclosing_sphere() const {
    if(_half_angle > math::pi<float> * 0.25f) {
        return {
            std::cos(_half_angle) * _range,
            std::sin(_half_angle) * _range
        };
    }
    const float half_radius = _range * 0.5f;
    const float dist = half_radius / std::cos(_half_angle);
    return {dist, dist};
}

float& SpotLightComponent::half_angle() {
    return _half_angle;
}

float SpotLightComponent::half_angle() const {
    return _half_angle;
}

float SpotLightComponent::half_inner_angle() const {
    return _half_inner_angle;
}

float& SpotLightComponent::half_inner_angle() {
    return _half_inner_angle;
}

bool& SpotLightComponent::cast_shadow() {
    return _cast_shadow;
}

bool SpotLightComponent::cast_shadow() const {
    return _cast_shadow;
}

u32& SpotLightComponent::shadow_lod() {
    return _shadow_lod;
}

u32 SpotLightComponent::shadow_lod() const {
    return _shadow_lod;
}

AABB SpotLightComponent::aabb() const {
    const auto sphere = enclosing_sphere();
    return AABB::from_center_extent(math::Vec3(0.0f, sphere.dist_to_center, 0.0f), math::Vec3(sphere.radius));
}

math::Vec2 SpotLightComponent::attenuation_scale_offset() const {
    const float cos_outer = std::cos(_half_angle);
    const float cos_inner = std::cos(_half_inner_angle);
    const float scale = 1.0f / std::max(0.001f, cos_inner - cos_outer);
    return math::Vec2(scale, -cos_outer * scale);
}

void SpotLightComponent::inspect(ecs::ComponentInspector* inspector) {
    LocalLightBase::inspect(inspector);
    inspector->inspect("Outer angle", _half_angle,          0.0f, math::to_rad(90.0f), ecs::ComponentInspector::FloatRole::HalfAngle);
    inspector->inspect("Inner angle", _half_inner_angle,    0.0f, math::to_rad(90.0f), ecs::ComponentInspector::FloatRole::HalfAngle);
    inspector->inspect("Cast shadow", _cast_shadow);
    inspector->inspect("Shadow LoD", _shadow_lod, 8);
}


}


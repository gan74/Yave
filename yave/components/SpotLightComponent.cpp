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

#include "SpotLightComponent.h"

namespace yave {

SpotLightComponent::EnclosingSphere SpotLightComponent::enclosing_sphere() const {
    if(_half_angle > math::pi<float> * 0.25f) {
        return {
            std::cos(_half_angle) * _radius,
            std::sin(_half_angle) * _radius
        };
    }
    const float half_radius = _radius * 0.5f;
    const float dist = half_radius / std::cos(_half_angle);
    return {dist, dist};
}

math::Vec3& SpotLightComponent::color() {
    return _color;
}

const math::Vec3& SpotLightComponent::color() const {
    return _color;
}

float& SpotLightComponent::intensity() {
    return _intensity;
}

float SpotLightComponent::intensity() const {
    return _intensity;
}

float& SpotLightComponent::radius() {
    return _radius;
}

float SpotLightComponent::radius() const {
    return _radius;
}

float& SpotLightComponent::falloff() {
    return _falloff;
}

float SpotLightComponent::falloff() const {
    return _falloff;
}

float& SpotLightComponent::half_angle() {
    return _half_angle;
}

float SpotLightComponent::half_angle() const {
    return _half_angle;
}

float& SpotLightComponent::angle_exponent() {
    return _angle_exp;
}

float SpotLightComponent::angle_exponent() const {
    return _angle_exp;
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

}


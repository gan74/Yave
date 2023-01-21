/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "DirectionalLightComponent.h"

#include <yave/camera/Camera.h>

namespace yave {

math::Vec3& DirectionalLightComponent::color() {
    return _color;
}

const math::Vec3& DirectionalLightComponent::color() const {
    return _color;
}

math::Vec3& DirectionalLightComponent::direction() {
    return _direction;
}

const math::Vec3& DirectionalLightComponent::direction() const {
    return _direction;
}

float& DirectionalLightComponent::intensity() {
    return _intensity;
}

float DirectionalLightComponent::intensity() const {
    return _intensity;
}

bool& DirectionalLightComponent::cast_shadow() {
    return _cast_shadow;
}

bool DirectionalLightComponent::cast_shadow() const {
    return _cast_shadow;
}

u32& DirectionalLightComponent::shadow_lod() {
    return _shadow_lod;
}

u32 DirectionalLightComponent::shadow_lod() const {
    return _shadow_lod;
}

float& DirectionalLightComponent::cascade_distance() {
    return _cascade_distance;
}

float DirectionalLightComponent::cascade_distance() const {
    return _cascade_distance;
}

float& DirectionalLightComponent::shadow_base_bias() {
    return _shadow_base_bias;
}

float DirectionalLightComponent::shadow_base_bias() const {
    return _shadow_base_bias;
}

}


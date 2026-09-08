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

#include "PointLightComponent.h"

#include <yave/ecs/ComponentInspector.h>

namespace yave {

bool& PointLightComponent::cast_shadow() {
    return _cast_shadow;
}

bool PointLightComponent::cast_shadow() const {
    return _cast_shadow;
}

u32& PointLightComponent::shadow_lod() {
    return _shadow_lod;
}

u32 PointLightComponent::shadow_lod() const {
    return _shadow_lod;
}

AABB PointLightComponent::aabb() const {
    return AABB::from_center_extent({}, math::Vec3(_range * 2.0f));
}

void PointLightComponent::inspect(ecs::ComponentInspector* inspector) {
    LocalLightBase::inspect(inspector);
    inspector->inspect("Cast shadow", _cast_shadow);
    inspector->inspect("Shadow LoD", _shadow_lod, 8);
}

}

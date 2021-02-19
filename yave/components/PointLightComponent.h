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
#ifndef YAVE_COMPONENTS_POINTLIGHTCOMPONENT_H
#define YAVE_COMPONENTS_POINTLIGHTCOMPONENT_H

#include <yave/ecs/ecs.h>
#include <y/reflect/reflect.h>

#include "TransformableComponent.h"

namespace yave {

class PointLightComponent final : public ecs::RequiredComponents<TransformableComponent> {
    public:
        PointLightComponent() = default;

        math::Vec3& color();
        const math::Vec3& color() const;

        float& intensity();
        float intensity() const;

        float& radius();
        float radius() const;

        float& falloff();
        float falloff() const;

        y_reflect(_color, _intensity, _radius, _falloff)

    private:
        math::Vec3 _color = math::Vec3{1.0f};
        float _intensity = 1.0f;
        float _radius = 10.0f;
        float _falloff = 1.0f;
};

}

#endif // YAVE_COMPONENTS_POINTLIGHTCOMPONENT_H


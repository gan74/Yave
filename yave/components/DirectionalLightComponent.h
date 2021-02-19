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
#ifndef YAVE_COMPONENTS_DIRECTIONALLIGHTCOMPONENT_H
#define YAVE_COMPONENTS_DIRECTIONALLIGHTCOMPONENT_H

#include <yave/yave.h>
#include <y/reflect/reflect.h>

namespace yave {

class DirectionalLightComponent final {
    public:
        DirectionalLightComponent() = default;

        math::Vec3& color();
        const math::Vec3& color() const;

        math::Vec3& direction();
        const math::Vec3& direction() const;

        float& intensity();
        float intensity() const;

        y_reflect(_color, _direction, _intensity)

    private:
        math::Vec3 _color = math::Vec3{1.0f};
        math::Vec3 _direction = math::Vec3{0.0f, 0.0f, -1.0f};
        float _intensity = 1.0f;
};
}

#endif // YAVE_COMPONENTS_DIRECTIONALLIGHTCOMPONENT_H


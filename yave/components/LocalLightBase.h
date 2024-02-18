/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_COMPONENTS_LOCALLIGHTBASE_H
#define YAVE_COMPONENTS_LOCALLIGHTBASE_H

#include <yave/utils/pbr.h>

namespace yave {

class LocalLightBase  {

    public:
        math::Vec3& color() {
            return _color;
        }

        const math::Vec3& color() const {
            return _color;
        }

        // Intensity in lm/4pi
        float& intensity() {
            return _intensity;
        }

        float intensity() const {
            return _intensity;
        }

        float& range() {
            return _range;
        }

        float range() const {
            return _range;
        }

        float& min_radius() {
            return _min_radius;
        }

        float min_radius() const {
            return _min_radius;
        }

        float& falloff() {
            return _falloff;
        }

        float falloff() const {
            return _falloff;
        }


        void inspect(ecs::ComponentInspector* inspector);

    protected:
        math::Vec3 _color = math::Vec3{1.0f};
        float _intensity = 1.0f;
        float _range = 10.0f;
        float _min_radius = 0.01f;
        float _falloff = 1.0f;
};

}

#endif // YAVE_COMPONENTS_LOCALLIGHTBASE_H


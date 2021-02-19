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
#ifndef YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H
#define YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H

#include <yave/assets/AssetPtr.h>

#include <yave/graphics/images/IBLProbe.h>

namespace yave {

class AtmosphereComponent final {
    public:
        float planet_radius = 3.0f;
        float atmosphere_height = 2.0f;

        float density_falloff = 1.0f;
        float scattering_strength = 400.0f;

        math::Vec3 wavelengths = math::Vec3(700.0f, 530.0f, 440.0f);
};

}

#endif // YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H


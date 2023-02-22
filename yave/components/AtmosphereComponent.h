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
#ifndef YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H
#define YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H

#include <yave/assets/AssetPtr.h>

#include <yave/graphics/images/IBLProbe.h>

#include <yave/ecs/ecs.h>

namespace yave {

class AtmosphereComponent final {
    public:
        float planet_radius() const;
        float atmosphere_height() const;
        float density_falloff() const;
        float scattering_strength() const;
        float zero_altitude() const;

        ecs::EntityId sun() const;

        void inspect(ecs::ComponentInspector* inspector);

        y_reflect(AtmosphereComponent, _planet_radius, _atmosphere_height, _zero_altitude, _density_falloff, _scattering_strength, _sun)

    private:
        float _planet_radius = 6400.0f;
        float _atmosphere_height = 100.0f;
        float _zero_altitude = 6400.0f;

        float _density_falloff = 5.0f;
        float _scattering_strength = 0.1f;

        ecs::EntityId _sun;

};

}

#endif // YAVE_COMPONENTS_ATMOSPHERECOMPONENT_H


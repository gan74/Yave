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

#include "AtmosphereComponent.h"

#include <yave/ecs/ComponentInspector.h>

namespace yave {


float AtmosphereComponent::planet_radius() const {
    return _planet_radius;
}

float AtmosphereComponent::atmosphere_height() const {
    return _atmosphere_height;
}

float AtmosphereComponent::density_falloff() const {
    return _density_falloff;
}

float AtmosphereComponent::scattering_strength() const {
    return _scattering_strength;
}

float AtmosphereComponent::zero_altitude() const {
    return _zero_altitude;
}

ecs::EntityId AtmosphereComponent::sun() const {
    return _sun;
}

void AtmosphereComponent::inspect(ecs::ComponentInspector* inspector) {
    inspector->inspect("Planet radius", _planet_radius, ecs::ComponentInspector::FloatRole::DistanceKilometers);
    inspector->inspect("Atmosphere height", _atmosphere_height, ecs::ComponentInspector::FloatRole::DistanceKilometers);
    inspector->inspect("Sea level altitude", _zero_altitude, ecs::ComponentInspector::FloatRole::DistanceKilometers);
    inspector->inspect("Density falloff", _density_falloff, 0.0f);
    inspector->inspect("Scattering strengh", _scattering_strength, 0.0f);
    inspector->inspect("Sun", _sun, ecs::type_index<DirectionalLightComponent>());
}

}


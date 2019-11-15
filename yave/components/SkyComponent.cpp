/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "SkyComponent.h"

namespace yave {

DirectionalLightComponent& SkyComponent::sun() {
	return _sun;
}

const DirectionalLightComponent& SkyComponent::sun() const {
	return _sun;
}

float SkyComponent::planet_radius() const {
	return _planet_radius;
}

float& SkyComponent::planet_radius() {
	return _planet_radius;
}

float SkyComponent::atmosphere_radius() const {
	return _atmos_radius;
}

float& SkyComponent::atmosphere_radius() {
	return _atmos_radius;
}

const math::Vec3& SkyComponent::beta_rayleight() const {
	return _beta_rayleight;
}

math::Vec3& SkyComponent::beta_rayleight() {
	return _beta_rayleight;
}

}

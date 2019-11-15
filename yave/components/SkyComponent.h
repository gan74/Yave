/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef YAVE_COMPONENTS_SKYCOMPONENT_H
#define YAVE_COMPONENTS_SKYCOMPONENT_H

#include "DirectionalLightComponent.h"

namespace yave {

class SkyComponent final {
	public:
		static constexpr auto earth_beta = math::Vec3(5.5e-6f, 13.0e-6f, 22.4e-6f);

		SkyComponent() = default;

		DirectionalLightComponent& sun();
		const DirectionalLightComponent& sun() const;

		float planet_radius() const;
		float& planet_radius();

		float atmosphere_radius() const;
		float& atmosphere_radius();

		const math::Vec3& beta_rayleight() const;
		math::Vec3& beta_rayleight();


	private:
		DirectionalLightComponent _sun;

		math::Vec3 _beta_rayleight = earth_beta;
		float _planet_radius = 6360.0f * 1000.0f;
		float _atmos_radius = 6420.0f * 1000.0f;
};
}

#endif // YAVE_COMPONENTS_SKYCOMPONENT_H

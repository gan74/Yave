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
#ifndef YAVE_COMPONENTS_SPOTLIGHTCOMPONENT_H
#define YAVE_COMPONENTS_SPOTLIGHTCOMPONENT_H

#include <yave/ecs/ecs.h>

#include "TransformableComponent.h"

namespace yave {

class SpotLightComponent final : public ecs::RequiredComponents<TransformableComponent> {
	public:
		SpotLightComponent() = default;

		math::Vec3& color();
		const math::Vec3& color() const;

		float& intensity();
		float intensity() const;

		float& radius();
		float radius() const;

		float& falloff();
		float falloff() const;

		float& angle();
		float angle() const;

		float& angle_exponent();
		float angle_exponent() const;


	private:
		math::Vec3 _color = math::Vec3{1.0f};
		float _intensity = 1.0f;
		float _radius = 1.0f;
		float _falloff = 1.0f;
		float _angle = math::to_rad(45.0f);
		float _angle_exp = 2.0f;
};

}

#endif // YAVE_COMPONENTS_SPOTLIGHTCOMPONENT_H

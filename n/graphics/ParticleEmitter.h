/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_GRAPHICS_PARTICLEEMITTER
#define N_GRAPHICS_PARTICLEEMITTER

#include <n/math/RandomDistribution.h>
#include <n/math/PrecomputedRange.h>
#include <n/math/Vec.h>
#include "Particle.h"
#include "DynamicBuffer.h"
#include "Renderable.h"
#include "Transformable.h"

namespace n {
namespace graphics {

class ParticleEmitter : public Transformable, public Renderable
{
	public:
		static constexpr uint Unlimited = uint(-1);
		static uint getMaxParticles();

		ParticleEmitter(uint max = 128);
		virtual ~ParticleEmitter();

		virtual void render(RenderQueue &q, RenderFlag) override;
		virtual void update(float sec);

		void setPositionDistribution(math::RandomDistribution<math::Vec3> *p);
		void setVelocityDistribution(math::RandomDistribution<math::Vec3> *v);
		void setLifeDistribution(math::RandomDistribution<float> *l);
		void setSizeOverLife(const math::PrecomputedRange<math::Vec2> &s);

		float getFlow() const;
		uint getTank() const;
		uint getMaxSize() const;

		void setFlow(float f);
		void setTank(uint t);

		Particle emit();

	private:
		uint computeEmition(float sec);
		math::Vec3 evalPosition();
		math::Vec3 evalVelocity();
		float evalDLife();

		UniformBuffer<Particle> particles;

		float flow;
		uint tank;

		float fraction;

		math::RandomDistribution<math::Vec3> *positions;
		math::RandomDistribution<math::Vec3> *velocities;
		math::RandomDistribution<float> *lives;
		math::PrecomputedRange<math::Vec2> sizes;

};

}
}

#endif // N_GRAPHICS_PARTICLEEMITTER


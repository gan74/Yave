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

#include <n/math/PrecomputedDistribution.h>
#include <n/math/Vec.h>
#include "Particle.h"

namespace n {
namespace graphics {

class ParticleEmitter
{
	typedef math::PrecomputedDistribution<math::Vec3> F3;
	typedef math::PrecomputedDistribution<float> F1;

	public:
		static constexpr uint Unlimited = uint(-1);
		ParticleEmitter(const F3 &position, const F3 &velocity, const F1 &life) : flow(10), fraction(0), tank(Unlimited), index(7), positions(position), velocities(velocity), lives(life) {
		}

		float getFlow() const {
			return flow;
		}

		uint getTank() const {
			return tank;
		}

		void setFlow(float f) {
			flow = f;
		}

		void setTank(uint t) {
			tank = t;
		}

		Particle emit() {
			Particle p;
			p.position = math::Vec4(positions[index], 1.0);
			p.velocity =  math::Vec4(velocities[index], 0.0);
			p.life = 1;
			p.dLife = 1.0 / lives[index];
			index++;
			return p;
		}

		uint computeEmition(float sec) {
			float f = std::min(float(tank), flow * sec + fraction);
			uint intFlow = f;
			fraction = f - intFlow;
			if(tank != Unlimited) {
				tank -= intFlow;
			}
			return intFlow;
		}


	private:
		float flow;
		float fraction;

		uint tank;

		uint index;
		math::PrecomputedDistribution<math::Vec3> positions;
		math::PrecomputedDistribution<math::Vec3> velocities;
		math::PrecomputedDistribution<float> lives;
};

}
}

#endif // N_GRAPHICS_PARTICLEEMITTER


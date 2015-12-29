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
		ParticleEmitter(const F3 &pos, const F3 &vel, const F &li) : index(7), spawn(pos), speed(vel), life(li) {
		}

		Particle emit() {
			Particle p;
			p.position = spawn[index];
			p.speed = speed[index];
			p.life = 1;
			p.lifeSpeed = 1.0 / life[index];
			index++;
		}


	private:
		uint index;
		math::PrecomputedDistribution<math::Vec3> spawn;
		math::PrecomputedDistribution<math::Vec3> speed;
		math::PrecomputedDistribution<float> life;
};

}
}

#endif // N_GRAPHICS_PARTICLEEMITTER


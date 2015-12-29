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

#ifndef N_GRAPHICS_PARTICLESYSTEM
#define N_GRAPHICS_PARTICLESYSTEM

#include <n/utils.h>
#include "Particle.h"
#include "DynamicBuffer.h"
#include "Renderable.h"
#include "Transformable.h"
#include "ParticleEmitter.h"

namespace n {
namespace graphics {

class ParticleSystem : public Transformable, public Renderable
{
	public:
		static uint getMaxParticles();

		ParticleSystem(uint max = 128);

		void addEmiter(const ParticleEmitter &emitter);

		virtual void render(RenderQueue &q, RenderFlag) override;

		virtual void update(float sec);

	private:
		void draw();

		core::Array<ParticleEmitter> emitters;
		UniformBuffer<Particle> particles;


};

}
}

#endif // N_GRAPHICS_PARTICLESYSTEM


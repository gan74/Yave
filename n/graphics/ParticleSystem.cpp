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

#include "ParticleSystem.h"
#include "ShaderInstance.h"

#include <iostream>

namespace n {
namespace graphics {

static ShaderInstance *getShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		shader = new ShaderInstance(
			new Shader<FragmentShader>(
				"out vec4 n_0;"
				"out vec4 n_1;"
				"out vec4 n_2;"
				"void main() {"
					"vec4 color = vec4(1, 0, 0, 1);"
					"float metal = 0;"
					"float roughness = 1.0;"
					"vec3 normal = vec3(0, 0, 1);"
					"n_0 = n_gbuffer0(color, normal, roughness, metal);"
					"n_1 = n_gbuffer1(color, normal, roughness, metal);"
					"n_2 = n_gbuffer2(color, normal, roughness, metal);"
				"}"),
			new Shader<VertexShader>(
				Particle::toShader() +

				"const uint MAX = 1024;"

				"uniform n_ParticleBuffer { "
					"Particle particles[MAX];"
				"};"

				"uniform mat4 n_ViewProjectionMatrix;"

				"out Particle particle;"

				"void main() {"
					"particle = particles[gl_VertexID];"
					"particle.position = n_ViewProjectionMatrix * particle.position;"
					"gl_Position = vec4(0.0);"
				"}"),
			new Shader<GeometryShader>(
				Particle::toShader() +
				"layout(points) in;"
				"layout(triangle_strip, max_vertices = 4) out;"

				"uniform vec2 n_ViewportSize;"

				"in Particle particle[1];"

				"vec2 quad[4] = vec2[](vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1));"

				"void main() {"
					"if(particle[0].life > 0) {"
						"float yRatio = n_ViewportSize.x / n_ViewportSize.y;"
						"for(int i = 0; i != 4; i++) {"
							"gl_Position = particle[0].position + vec4(quad[i].x, quad[i].y * yRatio, 0, 0);"
							"EmitVertex();"
						"}"
					"}"
				"}"
			));
	}
	return shader;
}

uint ParticleSystem::getMaxParticles() {
	return UniformBuffer<Particle>::getMaxSize();
}

ParticleSystem::ParticleSystem(uint max) : Transformable(), Renderable(), particles(max) {
	for(uint i = 0; i != particles.size(); i++) {
		particles[i].life = -1.0;
	}
}

void ParticleSystem::addEmiter(const ParticleEmitter &emitter) {
	emitters.append(emitter);
}

void ParticleSystem::render(RenderQueue &q, RenderFlag) {
	q.insert([=](RenderFlag) {
		draw();
	});
}

void ParticleSystem::update(float sec) {
	uint size = particles.size();
	core::Array<Particle> emitted;

	for(uint e = 0; e != emitters.size(); e++) {
		uint emition = emitters[e].computeEmition(sec);
		for(uint i = 0; i != emition; i++) {
			emitted.append(emitters[e].emit());
			emitted.last().material = e;
		}
	}
	for(uint i = 0; i != size; i++) {
		if(particles[i].life < 0) {
			if(!emitted.isEmpty()) {
				particles[i] = emitted.last();
				emitted.pop();
			}
		} else {
			particles[i].position += particles[i].velocity * sec;
			particles[i].velocity += math::Vec4(0, 0, -9.8, 0) * sec;
			particles[i].life -= particles[i].dLife * sec;
		}
	}
}

void ParticleSystem::draw() {
	ShaderInstance *sh = getShader();
	sh->bind();

	sh->setBuffer("n_ParticleBuffer", particles);

	MaterialRenderData rd;
	rd.cullMode = DontCull;
	rd.bind();
	ShaderInstance::validateState();

	gl::drawNoAttrib(particles.size());
}


}
}

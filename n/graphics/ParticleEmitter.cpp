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

#include "ParticleEmitter.h"

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

				"out Particle particle;"

				"void main() {"
					"particle = particles[gl_VertexID];"
					"gl_Position = vec4(0.0);"
				"}"),
			new Shader<GeometryShader>(
				Particle::toShader() +
				"layout(points) in;"
				"layout(triangle_strip, max_vertices = 4) out;"

				"uniform vec2 n_ViewportSize;"
				"uniform mat4 n_ViewProjectionMatrix;"

				"out vec3 n_Position;"
				"out vec4 n_ScreenPosition;"
				"out vec2 n_TexCoord;"

				"in Particle particle[1];"

				"vec2 quad[4] = vec2[](vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1));"
				"vec2 coords[4] = vec2[](vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1));"

				"void main() {"
					"n_InstanceID = 0;"
					"if(particle[0].life > 0) {"
						"float yRatio = n_ViewportSize.x / n_ViewportSize.y;"
						"vec4 basePos = n_ScreenPosition = n_ViewProjectionMatrix * vec4(n_Position = particle[0].position, 1.0);"

						"vec2 size = vec2(particle[0].size.x, particle[0].size.y * yRatio);"
						"for(int i = 0; i != 4; i++) {"
							"n_TexCoord = coords[i];"
							"gl_Position = basePos + vec4(quad[i] * size, 0, 0);"
							"EmitVertex();"
						"}"
					"}"
				"}"
			));
	}
	return shader;
}

uint ParticleEmitter::getMaxParticles() {
	return UniformBuffer<Particle>::getMaxSize();
}

ParticleEmitter::ParticleEmitter(uint max) : particles(max), flow(10), tank(Unlimited), fraction(0), positions(0), velocities(0), lives(0), sizes(math::Vec2(0.1)) {
	for(uint i = 0; i != particles.size(); i++) {
		particles[i].life = -1.0;
	}
}

ParticleEmitter::~ParticleEmitter() {
	delete positions;
	delete velocities;
	delete lives;
}

void ParticleEmitter::setPositionDistribution(math::RandomDistribution<math::Vec3> *p) {
	delete positions;
	positions = p;
}

void ParticleEmitter::setVelocityDistribution(math::RandomDistribution<math::Vec3> *v) {
	delete velocities;
	velocities = v;
}

void ParticleEmitter::setLifeDistribution(math::RandomDistribution<float> *l) {
	delete lives;
	lives = l;
}

void ParticleEmitter::setSizeOverLife(const math::PrecomputedRange<math::Vec2> &s) {
	sizes = s;
}

float ParticleEmitter::getFlow() const {
	return flow;
}

uint ParticleEmitter::getTank() const {
	return tank;
}

uint ParticleEmitter::getMaxSize() const {
	return particles.size();
}

void ParticleEmitter::setFlow(float f) {
	flow = f;
}

void ParticleEmitter::setTank(uint t) {
	tank = t;
}

uint ParticleEmitter::computeEmition(float sec) {
	float f = std::min(float(tank), flow * sec + fraction);
	uint intFlow = f;
	fraction = f - intFlow;
	if(tank != Unlimited) {
		tank -= intFlow;
	}
	return intFlow;
}

math::Vec3 ParticleEmitter::evalPosition() {
	return positions ? positions->eval() : math::Vec3(0);
}

math::Vec3 ParticleEmitter::evalVelocity() {
	return velocities ? velocities->eval() : math::Vec3(0);
}

float ParticleEmitter::evalDLife() {
	return 1.0 / (lives ? lives->eval() : 1);
}

void ParticleEmitter::render(RenderQueue &q, RenderFlag) {
	q.insert([=](RenderFlag) {
		ShaderInstance *sh = getShader();
		sh->bind();

		sh->setBuffer("n_ParticleBuffer", particles);

		MaterialRenderData rd;
		rd.cullMode = DontCull;
		rd.blendMode = SrcAlpha;
		rd.bind();
		ShaderInstance::validateState();

		gl::drawNoAttrib(particles.size());
	});
}

void ParticleEmitter::update(float sec) {
	uint size = getMaxSize();
	uint emitted = computeEmition(sec);

	for(uint i = 0; i != size; i++) {
		if(particles[i].life < 0) {
			if(emitted) {
				particles[i] = emit();
				particles[i].position += particles[i].velocity * sec * random();
				emitted--;
			}
		} else {
			particles[i].position += particles[i].velocity * sec;
			particles[i].size = sizes(1.0 - particles[i].life);
			//particles[i].velocity += math::Vec4(0, 0, -9.8, 0) * sec;
			particles[i].life -= particles[i].dLife * sec;
		}
	}
	if(tank != Unlimited) {
		tank += emitted;
	}
}

Particle ParticleEmitter::emit() {
	Particle p;
	p.position = evalPosition();
	p.velocity =  evalVelocity();
	p.life = 1;
	p.dLife = evalDLife();
	return p;
}

}
}

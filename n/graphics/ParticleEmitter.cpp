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
#include "FrameBuffer.h"

#include <iostream>

namespace n {
namespace graphics {

static Shader<FragmentShader> *getDefaultFrag() {
	static Shader<FragmentShader> *shader = 0;
	if(!shader) {
		shader = new Shader<FragmentShader>(
			"out vec4 n_0;"
			"out vec4 n_1;"
			"out vec4 n_2;"

			"in vec4 n_ParticleColor;"

			"in vec2 n_TexCoord;"

			"void main() {"
				"vec4 color = n_ParticleColor;"
				/*"float l = length(n_TexCoord - vec2(0.5)) * 2.0;"
				"vec4 color = vec4(saturate(1.0 - l));"*/
				"float metal = 0;"
				"float roughness = 1.0;"
				"vec3 normal = vec3(0, 0, 1);"
				"n_0 = n_gbuffer0(color, normal, roughness, metal);"
				"n_1 = n_gbuffer1(color, normal, roughness, metal);"
				"n_2 = n_gbuffer2(color, normal, roughness, metal);"
			"}");
	}
	return shader;
}

static Shader<VertexShader> *getDefaultVert() {
	static Shader<VertexShader> *shader = 0;
	if(!shader) {
		shader = new Shader<VertexShader>(
			Particle::toShader() +
			"const uint MAX = 1024;"

			"uniform n_ParticleBuffer { "
				"Particle particles[MAX];"
			"};"

			"out Particle particle;"

			"void main() {"
				"particle = particles[gl_VertexID];"
				"gl_Position = vec4(0.0);"
			"}");
	}
	return shader;
}


static Shader<GeometryShader> *getDefaultGeom(ParticleEmitter::ParticleFlags flags) {
	static Shader<GeometryShader> *shaders[ParticleEmitter::MaxFlags] = {0};
	if(!shaders[flags]) {
		shaders[flags] = new Shader<GeometryShader>(
			Particle::toShader() +
			"layout(points) in;"
			"layout(triangle_strip, max_vertices = 4) out;"

			"uniform vec2 n_ViewportSize;"
			"uniform mat4 n_ViewProjectionMatrix;"
			"uniform vec3 n_Forward;"

			"uniform sampler2D n_SizeOverLife;"
			"uniform sampler2D n_ColorOverLife;"

			"out vec3 n_Position;"
			"out vec4 n_ScreenPosition;"
			"out vec2 n_TexCoord;"
			"out vec4 n_ParticleColor;"

			"in Particle particle[1];"

			"vec2 quad[4] = vec2[](vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1));"
			"vec2 coords[4] = vec2[](vec2(0, 1), vec2(0, 0), vec2(1, 1), vec2(1, 0));"

			"vec4 sampleOverLife(sampler2D smp, float iLife) {"
				"float si = textureSize(smp, 0).x;"
				"float offset = 0.5 / si;"
				"float range = (si - 1) / si;"
				"return texture(smp, vec2(iLife * range + offset, 0.5));"
			"}"

			"void main() {"
				"n_InstanceID = 0;"
				"if(particle[0].life > 0) {"
					"float yRatio = n_ViewportSize.x / n_ViewportSize.y;"
					"float iLife = 1.0 - saturate(particle[0].life);"
					"vec2 size = sampleOverLife(n_SizeOverLife, iLife).xy;"
					"size.y *= yRatio;"
					"n_ParticleColor = sampleOverLife(n_ColorOverLife, iLife);"
					"vec4 basePos = n_ViewProjectionMatrix * vec4(n_Position = particle[0].position, 1.0);"
					"vec3 side = normalize(cross(n_Forward, particle[0].velocity));"
					"vec3 vel = particle[0].velocity;"
					"float vLen = max(epsilon, length(vel));"
					"vel /= vLen;"
					+ (flags & ParticleEmitter::VelocityScaled ? "size *= vLen;" : "") +
					"for(int i = 0; i != 4; i++) {"
						"n_TexCoord = coords[i];"
						"vec2 v = quad[i] * size;"
						+ (flags & ParticleEmitter::VelocityOriented
							? "gl_Position = n_ScreenPosition = n_ViewProjectionMatrix * vec4(particle[0].position + v.x * vel + v.y * side, 1);"
							: "gl_Position = n_ScreenPosition = basePos + vec4(v, 0, 0);"
						) +
						"EmitVertex();"
					"}"
				"}"
			"}");
	}
	return shaders[flags];
}

uint ParticleEmitter::getMaxParticles() {
	return UniformBuffer<Particle>::getMaxSize();
}

ParticleEmitter::ParticleEmitter(uint max) : particles(max), flow(10), tank(Unlimited), fraction(0), positions(0), velocities(0), lives(0), drag(0), flags(None) {
	radius = -1;
	#warning ParticleEmitter has infinite radius
	MaterialData md;
	md.render.blendMode = SrcAlpha;
	md.render.cullMode = DontCull;
	material = Material(md);
	setSizeOverLife(math::Vec2(0.1));
	setColorOverLife(core::Array<Color<>>({Red, Blue}));
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
	setSizeOverLife(Texture(new ImageData(math::Vec2ui(s.size(), 1), ImageFormat::RG32F, s.begin())));
}

void ParticleEmitter::setSizeOverLife(const Texture &s) {
	sizes = s;
}

void ParticleEmitter::setColorOverLife(const math::PrecomputedRange<Color<>> &c) {
	setColorOverLife(Texture(new ImageData(math::Vec2ui(c.size(), 1), ImageFormat::RGBA32F, c.begin())));
}

void ParticleEmitter::setColorOverLife(const Texture &c) {
	colors = c;
}

void ParticleEmitter::setAcceleration(const math::Vec3 &f) {
	accel = f;
}

void ParticleEmitter::setDrag(float d) {
	drag = d;
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

void ParticleEmitter::addModifier(Modifier *m) {
	modifiers.append(m);
}

void ParticleEmitter::setMaterial(Material mat) {
	material = mat;
}

void ParticleEmitter::setFlags(ParticleFlags fl) {
	flags = fl;
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
	return getPosition() + (positions ? getRotation()(positions->eval()) : math::Vec3(0));
}

math::Vec3 ParticleEmitter::evalVelocity() {
	return velocities ? getRotation()(velocities->eval()) : math::Vec3(0);
}

float ParticleEmitter::evalDLife() {
	return 1.0 / (lives ? lives->eval() : 1);
}

void ParticleEmitter::render(RenderQueue &q, RenderFlag) {
	q.insert([=](RenderFlag) {
		Shader<FragmentShader> *frag = getDefaultFrag()->bindAsDefault();
		Shader<VertexShader> *vert = getDefaultVert()->bindAsDefault();
		Shader<GeometryShader> *geom = getDefaultGeom(flags)->bindAsDefault();

		material->bind();
		const ShaderInstance *sh = ShaderInstance::getCurrent();

		sh->setBuffer("n_ParticleBuffer", particles);
		sh->setValue("n_SizeOverLife", sizes, TextureSampler::Bilinear | TextureSampler::Clamp);
		sh->setValue("n_ColorOverLife", colors, TextureSampler::Bilinear | TextureSampler::Clamp);
		sh->setValue("n_Forward", -GLContext::getContext()->getViewMatrix()[2].sub(3));

		ShaderInstance::validateState();

		gl::drawNoAttrib(particles.size());

		Shader<FragmentShader>::setDefault(frag);
		Shader<VertexShader>::setDefault(vert);
		Shader<GeometryShader>::setDefault(geom);
	});
}

void ParticleEmitter::update(float sec) {
	uint size = getMaxSize();
	uint emitted = computeEmition(sec);

	math::Vec3 f = accel * sec;
	for(uint i = 0; i != size; i++) {
		if(particles[i].life < 0) {
			if(emitted) {
				particles[i] = emit();
				particles[i].position += particles[i].velocity * sec * random();
				emitted--;
			}
		} else {
			particles[i].position += particles[i].velocity * sec;
			particles[i].life -= particles[i].dLife * sec;
			particles[i].velocity += f;
		}
	}
	float d = drag * sec;
	if(drag) {
		for(uint i = 0; i != size; i++) {
			particles[i].velocity -= particles[i].velocity * std::min(1.0f, particles[i].velocity.length() * d); // should be length2 but too strong
		}
	}
	for(Modifier *m : modifiers) {
		for(uint i = 0; i != size; i++) {
			m->modify(particles[i], sec);
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

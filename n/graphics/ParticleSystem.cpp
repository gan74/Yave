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

namespace n {
namespace graphics {

static ShaderInstance *getShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		core::String partStruct = "struct Particle {"
				"vec4 position;"
				"vec4 velocity;"
				"float life;"
			"};";
		shader = new ShaderInstance(
			new Shader<FragmentShader>(
				"out vec4 n_Out;"
				"void main() {"
					"n_Out = vec4(1, 0, 0, 1);"
				"}"),
			new Shader<VertexShader>(
				partStruct +

				"const uint MAX = 1024;"

				"uniform n_ParticleBuffer { "
					"Particle particles[MAX];"
				"};"

				"uniform mat4 n_ViewProjectionMatrix;"

				"out Particle particle;"

				"void main() {"
					"particle.position = n_ViewProjectionMatrix * vec4(particles[gl_VertexID].position.xyz, 1.0);"
					"gl_Position = vec4(0.0);"
				"}"),
			new Shader<GeometryShader>(
				partStruct +
				"layout(points) in;"
				"layout(triangle_strip, max_vertices = 4) out;"

				"uniform vec2 n_ViewportSize;"


				"in Particle particle[1];"

				"vec2 quad[4] = vec2[](vec2(-1, 1), vec2(-1, -1), vec2(1, 1), vec2(1, -1));"

				"void main() {"
					"float yRatio = n_ViewportSize.x / n_ViewportSize.y;"
					"for(int i = 0; i != 4; i++) {"
						"gl_Position = particle[0].position + vec4(quad[i].x, quad[i].y * yRatio, 0, 0);"
						"EmitVertex();"
					"}"
				"}"
			));
	}
	return shader;
}

ParticleSystem::ParticleSystem(uint max) : Transformable(), Renderable(), particles(max) {
	for(uint i = 0; i != max; i++) {
		float a = random() * 2 * math::pi;
		float b = random() * 2 * math::pi;
		float r = random() * 50 + 25;
		particles[i].position = math::Vec4();
		particles[i].velocity = math::Vec4(r * cos(a) * sin(b), r * sin(a) * sin(b), r * cos(b), 0);
	}
}

void ParticleSystem::render(RenderQueue &q, RenderFlag) {
	q.insert([=](RenderFlag) {
		draw();
	});
}

void ParticleSystem::update(double sec) {
	uint size = particles.size();
	for(uint i = 0; i != size; i++) {
		particles[i].position += particles[i].velocity * sec;
		particles[i].velocity += math::Vec4(0, 0, -9.8, 0) * sec;
	}
}

void ParticleSystem::draw() {
	ShaderInstance *sh = getShader();
	sh->bind();

	sh->setBuffer("n_ParticleBuffer", particles);

	MaterialRenderData rd;
	rd.depthTested = false;
	rd.cullMode = DontCull;
	rd.bind();
	ShaderInstance::validateState();

	gl::drawNoAttrib(particles.size());
}


}
}

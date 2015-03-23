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

#include "DeferredShadingRenderer.h"
#include "VertexArrayObject.h"
#include "GL.h"

#include <iostream>

namespace n {
namespace graphics {

DeferredShadingRenderer::DeferredShadingRenderer(GBufferRenderer *c, const math::Vec2ui &s) : BufferedRenderer(s.isNull() ? c->getFrameBuffer().getSize() : s), child(c) {
	buffer.setAttachmentEnabled(0, true);
	buffer.setDepthEnabled(true);
}

void *DeferredShadingRenderer::prepare() {
	(*child)();
	return 0;
}

void DeferredShadingRenderer::render(void *) {
	buffer.bind();
	gl::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	ShaderCombinaison *sh = getShader();
	sh->bind();
	sh->setValue("n_0", child->getFrameBuffer().getAttachement(0));
	sh->setValue("n_1", child->getFrameBuffer().getAttachement(1));
	sh->setValue("n_D", child->getFrameBuffer().getDepthAttachement());
	GLContext::getContext()->getScreen().draw(VertexAttribs());
}

ShaderCombinaison *DeferredShadingRenderer::getShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		Shader<FragmentShader> *frag = new Shader<FragmentShader>("#version 420\n"
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"
			"in vec2 texCoord;"

			"out vec4 color;"

			"void main() {"
				"vec3 normal = texture(n_1, texCoord).xyz * 2 - 1;"
				"float NoL = dot(normal, normalize(vec3(1, 1, 1)));"
				"color = texture(n_0, texCoord) * NoL;"
			"}");

		Shader<VertexShader> *vert = new Shader<VertexShader>("#version 420\n"
			"layout(location = 0) in vec3 n_VertexPosition;"
			"layout(location = 1) in vec3 n_VertexNormal;"
			"layout(location = 2) in vec3 n_VertexTangent;"
			"layout(location = 3) in vec2 n_VertexCoord;"

			"out vec2 texCoord;"

			"void main() {"
				"gl_Position = vec4(n_VertexPosition, 1.0);"
				"texCoord = n_VertexCoord;"
			"}");
		shader = new ShaderCombinaison(frag, vert);
		if(!shader->getLogs().isEmpty()) {
			std::cerr<<shader->getLogs()<<std::endl;
		}
	}
	return shader;
}

}
}

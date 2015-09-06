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

#include "ExponentialShadowRenderer.h"

namespace n {
namespace graphics {

ShaderCombinaison *getExpShader(float exp) {
	core::Map<float, ShaderCombinaison *> shaders;
	ShaderCombinaison *sh = shaders.get(exp, 0);
	if(!sh) {
		sh = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"

			"in vec2 n_TexCoord;"
			"out float n_Out;"

			"void main() {"
				"float depth = texture(n_0, n_TexCoord).x;"
				/*"float dx = dFdx(depth);"
				"float dy = dFdy(depth);"
				"depth += (dx * dx + dy * dy);"*/
				"n_Out = exp(" + core::String(abs(exp)) + " * depth);"
			"}"), ShaderProgram::NoProjectionShader);
		shaders[exp] = sh;
	}
	return sh;
}


ExponentialShadowRenderer::ExponentialShadowRenderer(ShadowRenderer *c, uint fHStep, float exp) : ShadowRenderer(c->getSize().x()), child(c), exponent(exp), blurs{BlurBufferRenderer::createBlurShader(false, fHStep), BlurBufferRenderer::createBlurShader(true, fHStep)} {
	mapIndex = 0;
	shaderCode = "vec3 proj = projectShadow(pos);"
				 "float eD = exp(-" + core::String(abs(exp)) + " * proj.z);"
				 "float depth = texture(n_LightShadow, proj.xy).x;"
				 "return clamp((eD * depth), 0.0, 1.0);";
}

void ExponentialShadowRenderer::createBuffer() {
	if(!buffer) {
		buffer = GLContext::getContext()->getFrameBufferPool().get(getSize(), false, ImageFormat::R32F);
	}
}

void ExponentialShadowRenderer::render(void *ptr) {
	createBuffer();
	child->render(ptr);

	buffer->bind();
	ShaderCombinaison *sh = getExpShader(exponent);
	sh->setValue("n_0", child->getShadowMap());
	sh->bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	FrameBuffer *temp = GLContext::getContext()->getFrameBufferPool().get(getSize(), false, ImageFormat::R32F);

	blurs[1]->setValue("n_0", temp->getAttachement(0));
	blurs[0]->setValue("n_0", buffer->getAttachement(0));

	temp->bind();
	blurs[0]->bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	buffer->bind();
	blurs[1]->bind();
	GLContext::getContext()->getScreen().draw(Material(), VertexAttribs(), RenderFlag::NoShader);

	blurs[1]->unbind();
	GLContext::getContext()->getFrameBufferPool().add(temp);
	child->poolBuffer();
}

}
}

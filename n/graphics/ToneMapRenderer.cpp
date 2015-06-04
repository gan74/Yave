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

#include "ToneMapRenderer.h"
#include "ShaderCombinaison.h"
#include "VertexArrayObject.h"
#include <n/utils.h>
#include <iostream>

namespace n {
namespace graphics {

ShaderCombinaison *getToneShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"

			"in vec2 n_TexCoord;"

			"out vec4 n_Out;"

			"float rgbLum(vec3 rgb) {"
				"return dot(vec3(0.299, 0.587, 0.114), rgb);"
			"}"

			"vec3 reinhard(vec3 col, float exp, float key, float white) {"
				"float cL = rgbLum(col);"
				"float lum = cL * exp / key;"
				"float W = 1 + lum / sqr(white);"
				"lum *= W / (lum + 1);"
				"return col * lum / cL;"
			"}"

			"void main() {"
				"vec4 tex = textureGather(n_1, (0.5 / textureSize(n_1, 0)), 0);"
				"float lum = exp((tex.x + tex.y + tex.z + tex.w) * 0.25);"
				"vec4 color = texture(n_0, n_TexCoord);"
				"n_Out = vec4(reinhard(color.rgb, 0.33, lum, 1.0), color.a);"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

ShaderCombinaison *getLogShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"
			"in vec2 n_TexCoord;"
			"out float n_Out;"

			"float rgbLum(vec3 rgb) {"
				"return dot(vec3(0.299, 0.587, 0.114), rgb);"
			"}"


			"const float epsilon = 0.0001;"

			"void main() {"
				"vec4 color = texture(n_0, n_TexCoord);"
				"n_Out = max(0.0, log(epsilon + rgbLum(color.rgb)));"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

ShaderCombinaison *getDSShader() {
	static ShaderCombinaison *shader = 0;
	if(!shader) {
		shader = new ShaderCombinaison(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"
			"in vec2 n_TexCoord;"
			"out float n_Out;"
			"uniform float scale;"

			"vec2 rescale(vec2 tex) {"
				"tex *= scale;"
				"vec2 h = vec2(0);  (0.5 / textureSize(n_0, 0));"
				"return tex * (1.0 - 2 * h) + h;"
			"}"

			"void main() {"
				"vec4 tex = textureGather(n_0, rescale(n_TexCoord), 0);"
				"n_Out = (tex.x + tex.y + tex.z + tex.w) * 0.25;"
			"}"), new Shader<VertexShader>(
			"layout(location = 0) in vec3 n_VertexPosition;"
			"layout(location = 3) in vec2 n_VertexCoord;"

			"uniform float scale;"

			"out vec2 n_TexCoord;"

			"void main() {"
				"vec3 vp = n_VertexPosition * 0.5 + 0.5;"
				"vp *= 0.5 * scale;"
				"vp = vp * 2.0 - 1.0;"
				"gl_Position = vec4(vp, 1.0);"
				"n_TexCoord = n_VertexCoord;"
			"}"));
	}
	return shader;
}

Texture computeLum(const Texture &in, FrameBuffer *buffers[]) {
	ShaderCombinaison *sh = getLogShader();
	sh->bind();
	sh->setValue("n_0", in);

	if(buffers[1]->isModified()) {
		buffers[1]->bind();
	}
	buffers[0]->bind();
	GLContext::getContext()->getScreen().draw(VertexAttribs());

	sh = getDSShader();
	sh->bind();

	float scale = 1.0;
	bool last = false;
	uint baseSize = buffers[0]->getSize().x();
	while(baseSize != 2) {
		sh->setValue("n_0", buffers[last]->getAttachement(0));
		sh->setValue("scale", scale);
		buffers[!last]->bind();
		last = !last;
		scale *= 0.5;
		baseSize /= 2;
		GLContext::getContext()->getScreen().draw(VertexAttribs());
	}
	return buffers[last]->getAttachement(0);
}

ToneMapRenderer::ToneMapRenderer(BufferedRenderer *c, uint s) : BufferableRenderer(), child(c), slot(s) {
	uint ls = core::log2ui(child->getFrameBuffer().getSize().min());
	math::Vec2ui size = math::Vec2ui(1 << ls);
	buffers[0] = new FrameBuffer(size);
	buffers[0]->setAttachmentEnabled(0, true);
	buffers[0]->setAttachmentFormat(0, ImageFormat::R32F);
	buffers[1] = new FrameBuffer(size);
	buffers[1]->setAttachmentEnabled(0, true);
	buffers[1]->setAttachmentFormat(0, ImageFormat::R32F);
}

ToneMapRenderer::~ToneMapRenderer() {
	delete buffers[0];
	delete buffers[1];
}

void *ToneMapRenderer::prepare() {
	return child->prepare();
}

void ToneMapRenderer::render(void *ptr) {
	const FrameBuffer *fb = GLContext::getContext()->getFrameBuffer();
	child->render(ptr);

	Texture lum = computeLum(child->getFrameBuffer().getAttachement(slot), buffers);

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}

	ShaderCombinaison *sh = getToneShader();
	sh->bind();
	sh->setValue("n_0", child->getFrameBuffer().getAttachement(slot));
	sh->setValue("n_1", lum);

	GLContext::getContext()->getScreen().draw(VertexAttribs());
}


}
}

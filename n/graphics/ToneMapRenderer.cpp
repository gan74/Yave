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

			"uniform float logMin;"
			"uniform float logMax;"
			"uniform float exposure;"
			"uniform float white;"

			"in vec2 n_TexCoord;"

			"out vec4 n_Out;"

			"float rgbLum(vec3 rgb) {"
				"return dot(vec3(0.299, 0.587, 0.114), rgb);"
			"}"

			"float clampLum(float l) {"
				"return clamp(l, logMin, logMax);"
			"}"

			"vec3 reinhard(vec3 col, float exp, float key, float w) {"
				"float cL = rgbLum(col);"
				"float lum = cL * exp / key;"
				"float W = 1 + lum / sqr(w);"
				"lum *= W / (lum + 1);"
				"return col * lum / cL;"
			"}"

			"vec3 gamma(vec3 col) {"
				"return pow(col, vec3(1.0 / 2.2));"
			"}"

			"void main() {"
				"vec2 offset = 0.5 / textureSize(n_1, 0);"
				"vec4 avgLum4 = textureGather(n_1, offset, 0);"
				//"vec4 maxLum4 = textureGather(n_1, offset, 1);"
				"float avgLum = exp(clampLum((avgLum4.x + avgLum4.y + avgLum4.z + avgLum4.w) * 0.25));"
				//"float maxLum = max(max(maxLum4.x, maxLum4.y), max(maxLum4.z, maxLum4.w));"

				"vec4 color = texture(n_0, n_TexCoord);"
				"vec3 rein = reinhard(color.rgb, exposure, avgLum, white);"
				"n_Out = vec4(rein, color.a);"

				"if(n_TexCoord.x < 0.1 && n_TexCoord.y > 0.9) { n_Out = vec4(avgLum); }"
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
				"float lum = rgbLum(color.rgb);"
				"n_Out = log(epsilon + lum);"
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
				"vec4 avgLum = textureGather(n_0, rescale(n_TexCoord), 0);"
				//"vec4 maxLum = textureGather(n_0, rescale(n_TexCoord), 1);"
				"n_Out.x = (avgLum.x + avgLum.y + avgLum.z + avgLum.w) * 0.25;"
				//"n_Out.y = max(max(maxLum.x, maxLum.y), max(maxLum.z, maxLum.w));"
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
	sh->setValue("logMin", log(0.055));
	sh->setValue("logMax", log(0.55));
	sh->setValue("exposure", 0.33);
	sh->setValue("white", 2.0);

	sh->setValue("n_0", child->getFrameBuffer().getAttachement(slot));
	sh->setValue("n_1", lum);
	/*float lums[2] = { -1, -1 };
	gl::glGetTextureSubImage(lum.getHandle(), 0, 0, 0, 0, 1, 1, 1, GL_RG, GL_FLOAT, 2 * sizeof(float), lums);
	std::cout<<lum.getHandle()<<"  max = "<<lums[1]<<", avg = "<<exp(lums[0])<<std::endl;*/

	GLContext::getContext()->getScreen().draw(VertexAttribs());
}


}
}

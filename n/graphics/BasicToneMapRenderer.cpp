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

#include "BasicToneMapRenderer.h"
#include "ShaderInstance.h"
#include "VertexArrayObject.h"
#include <n/utils.h>

namespace n {
namespace graphics {

static const Material &getMaterial() {
	static Material mat;
	if(mat.isNull()) {
		MaterialData i;
		i.fancy.depth = MaterialData::Always;
		mat = Material(i);
	}
	return mat;
}

static const Material &getLumMaterial() {
	static Material mat;
	if(mat.isNull()) {
		MaterialData i;
		i.fancy.blend = MaterialData::SrcAlpha;
		i.fancy.depth = MaterialData::Always;
		mat = Material(i);
	}
	return mat;
}

static ShaderInstance *getToneShader(bool debug = false) {
	static ShaderInstance *shaders[2] = {0, 0};
	if(!shaders[debug]) {
		static core::String deb[2] = {"", "\n#define DEBUG\n"};
		shaders[debug] = new ShaderInstance(new Shader<FragmentShader>(
			deb[debug] +
			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"

			"uniform float exposure;"
			"uniform float white;"

			"in vec2 n_TexCoord;"

			"out vec4 n_Out;"

			"float rgbLum(vec3 rgb) {"
				"return dot(vec3(0.299, 0.587, 0.114), rgb);"
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
				"float avgLum = texelFetch(n_1, ivec2(0), 0).x;"

				"vec4 color = texture(n_0, n_TexCoord);"
				"vec3 rein = reinhard(color.rgb, exposure, avgLum, white);"
				"n_Out = vec4(rein, color.a);"
				"\n#ifdef DEBUG\n"
				"if(color.x > 1 || color.y > 1 || color.z > 1) { n_Out = vec4(1, 0, 0, 1); }"
				"if(n_TexCoord.x < 0.1 && n_TexCoord.y > 0.9) { n_Out = vec4((avgLum)); }"
				"if(n_TexCoord.x > 0.5) { n_Out = color; }"
				"\n#endif\n"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shaders[debug];
}

static ShaderInstance *getLumShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		shader = new ShaderInstance(new Shader<FragmentShader>(
			"uniform sampler2D n_0;"

			"uniform float logMin;"
			"uniform float logMax;"
			"uniform float blend;"

			"in vec2 n_TexCoord;"
			"out vec4 n_Out;"

			"float clampLum(float l) {"
				"return clamp(l, logMin, logMax);"
			"}"

			"void main() {"
				"vec2 offset = 0.5 / textureSize(n_0, 0);"
				"vec4 avgLum4 = textureGather(n_0, offset, 0);"
				"n_Out = vec4(vec3(exp(clampLum((avgLum4.x + avgLum4.y + avgLum4.z + avgLum4.w) * 0.25))), blend);"
			"}"), ShaderProgram::NoProjectionShader);
	}
	return shader;
}


static ShaderInstance *getLogShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		shader = new ShaderInstance(new Shader<FragmentShader>(
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

static ShaderInstance *getDSShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		shader = new ShaderInstance(new Shader<FragmentShader>(
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
				"n_Out = (avgLum.x + avgLum.y + avgLum.z + avgLum.w) * 0.25;"
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

static Texture computeLum(const Texture &in, FrameBuffer *buffers[]) {
	ShaderInstance *sh = getLogShader();
	sh->setValue(SVTexture0, in);
	sh->bind();

	buffers[0]->bind();
	GLContext::getContext()->getScreen().draw(getMaterial(), VertexAttribs(), RenderFlag::NoShader);

	sh = getDSShader();
	sh->bind();

	float scale = 1.0;
	bool last = false;
	uint baseSize = buffers[0]->getSize().x();
	while(baseSize != 2) {
		sh->setValue(SVTexture0, buffers[last]->getAttachement(0));
		sh->setValue("scale", scale);
		buffers[!last]->bind();
		last = !last;
		scale *= 0.5;
		baseSize /= 2;
		GLContext::getContext()->getScreen().draw(getMaterial(), VertexAttribs(), RenderFlag::NoShader);
	}
	return buffers[last]->getAttachement(0);
}

BasicToneMapRenderer::BasicToneMapRenderer(BufferedRenderer *c, uint s) : BufferableRenderer(), child(c), luma(GLContext::getContext()->getFrameBufferPool().get(math::Vec2ui(1), true, ImageFormat::R32F)), slot(s), exposure(0.1), white(2), range(0.001, 0.5), adaptation(0.5), debug(false) {
}

BasicToneMapRenderer::~BasicToneMapRenderer() {
	GLContext::getContext()->getFrameBufferPool().add(luma);
}

void *BasicToneMapRenderer::prepare() {
	return child->prepare();
}

void BasicToneMapRenderer::render(void *ptr) {
	const FrameBuffer *fb = GLContext::getContext()->getFrameBuffer();
	child->render(ptr);

	uint ls = log2ui(child->getSize().min());
	math::Vec2ui size = math::Vec2ui(1 << ls);
	FrameBuffer *buffers[] = {GLContext::getContext()->getFrameBufferPool().get(size, false, ImageFormat::R32F),
							  GLContext::getContext()->getFrameBufferPool().get(size, false, ImageFormat::R32F)};

	Texture lum = computeLum(child->getFrameBuffer().getAttachement(slot), buffers);

	double dt = std::min(float(timer.reset()), adaptation);

	ShaderInstance *lumSh = getLumShader();
	lumSh->bind();
	lumSh->setValue(SVTexture0, lum);
	lumSh->setValue("logMin", log(range.x()));
	lumSh->setValue("logMax", log(range.y()));
	lumSh->setValue("blend", dt / adaptation);
	luma->bind();
	GLContext::getContext()->getScreen().draw(getLumMaterial(), VertexAttribs(), RenderFlag::NoShader);

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}

	ShaderInstance *sh = getToneShader(debug);
	sh->setValue("exposure", exposure);
	sh->setValue("white", white);

	sh->setValue(SVTexture0, child->getFrameBuffer().getAttachement(slot));
	sh->setValue(SVTexture1, luma->getAttachement(0));
	sh->bind();

	GLContext::getContext()->getScreen().draw(getMaterial(), VertexAttribs(), RenderFlag::NoShader);

	GLContext::getContext()->getFrameBufferPool().add(buffers[0]);
	GLContext::getContext()->getFrameBufferPool().add(buffers[1]);
}


}
}

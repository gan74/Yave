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

#include "DeferredIBLRenderer.h"
#include "ImageLoader.h"

#include <iostream>

namespace n {
namespace graphics {

Material getMaterial() {
	Material mat;
	if(mat.isNull()) {
		MaterialData data;
		data.fancy.depthTested = false;
		mat = Material(data);
	}
	return mat;
}


CubeMap *getCube() {
	static CubeMap *cube = 0;
	if(!cube) {
		cube = new CubeMap(
			(Image(ImageLoader::load<core::String>("skybox/top.tga"))),
			(Image(ImageLoader::load<core::String>("skybox/bottom.tga"))),
			(Image(ImageLoader::load<core::String>("skybox/right.tga"))),
			(Image(ImageLoader::load<core::String>("skybox/left.tga"))),
			(Image(ImageLoader::load<core::String>("skybox/front.tga"))),
			(Image(ImageLoader::load<core::String>("skybox/back.tga"))));
	}
	return cube;
}

ShaderInstance *getShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		shader = new ShaderInstance(new Shader<FragmentShader>(
			"uniform samplerCube n_Cube;"

			"uniform sampler2D n_1;"
			"uniform sampler2D n_2;"

			"in vec2 n_TexCoord;"
			"out vec4 n_Out;"
			"in vec3 n_View;"

			"void main() {"
				"vec4 material = texture(n_2, n_TexCoord);"
				"vec3 normal = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				"float metal = material.y;"
				"float roughness = material.x;"
				"n_Out = textureLod(n_Cube, normal, 8.0);"
			"}"
		), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

DeferredIBLRenderer::DeferredIBLRenderer(GBufferRenderer *c) : BufferableRenderer(), child(c) {
}

void *DeferredIBLRenderer::prepare() {
	return child->prepare();
}

void DeferredIBLRenderer::render(void *ptr) {
	const FrameBuffer *fb = GLContext::getContext()->getFrameBuffer();
	child->render(ptr);
	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}

	ShaderInstance *shader = getShader();
	shader->setValue("n_Cube", *getCube(), TextureSampler::Trilinear);
	shader->setValue(SVTexture1, child->getFrameBuffer().getAttachement(1));
	shader->setValue(SVTexture2, child->getFrameBuffer().getAttachement(2));

	shader->bind();
	GLContext::getContext()->getScreen().draw(getMaterial(), VertexAttribs(), RenderFlag::NoShader);
	shader->unbind();




}

}
}

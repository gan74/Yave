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
#include "DeferredCommon.h"
#include "CubeFrameBuffer.h"
#include "IBLProbe.h"

namespace n {
namespace graphics {

static MaterialRenderData getMaterial() {
	MaterialRenderData mat;
	mat.depthTested = false;
	mat.depthWrite = false;
	mat.blendMode = BlendMode::Add;
	return mat;
}


static IBLProbe *getCube() {
	static IBLProbe *cube = 0;
	if(!cube) {
		cube = new IBLProbe(CubeMap(
			(Image(ImageLoader::load<core::String>("./skybox/top.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/bottom.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/right.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/left.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/front.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/back.tga"))), false));
	}
	return cube;
}

static ShaderInstance *getShader() {
	static ShaderInstance *shader = 0;
	if(!shader) {
		shader = new ShaderInstance(new Shader<FragmentShader>(
			"\n#define MAX 1\n"

			+ IBLProbe::toShader() +


			"uniform probeBuffer { "
				"n_IBLProbe probes[MAX];"
			"};"


			"uniform sampler2D n_0;"
			"uniform sampler2D n_1;"
			"uniform sampler2D n_2;"
			"uniform sampler2D n_D;"

			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"

			"out vec4 n_Out;"

			"vec3 unproj(vec2 C, out float d) {"
				"vec4 VP = vec4(vec3(C, d = texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w;"
			"}"

			+ getBRDFs() +

			"void main() {"
				"float depth = 0;"
				"vec3 pos = unproj(n_TexCoord, depth);"
				"n_GBufferData gbuffer = n_unpackGBuffer(n_0, n_1, n_2, ivec2(gl_FragCoord.xy));"

				"vec3 view = normalize(pos - n_Cam);"
				"if(depth == 1) {"
					"n_Out = textureLod(probes[0].cube, view, 0);"
				"} else {"
					"n_Out = iblProbe(probes[0], reflect(view, gbuffer.normal), gbuffer.roughness);"
				"}"
			"}"
		), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

DeferredIBLRenderer::DeferredIBLRenderer(GBufferRenderer *c) : BufferableRenderer(), child(c), probes(1) {
}

void *DeferredIBLRenderer::prepare() {
	return child->prepare();
}

void DeferredIBLRenderer::render(void *ptr) {


	SceneRenderer::FrameData *sceneData = child->getSceneRendererData(ptr);
	math::Matrix4<> invCam = (sceneData->proj * sceneData->view).inverse();
	math::Vec3 cam = sceneData->camera->getPosition();

	const FrameBufferBase *fb = GLContext::getContext()->getFrameBuffer();
	child->render(ptr);

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}
	FrameBuffer::clear(true, true); // <<------------------------------------ REMOVE


	probes[0] = getCube()->toBufferData();

	ShaderInstance *shader = getShader();

	shader->setBuffer("probeBuffer", probes);
	shader->setValue("n_Inv", invCam);
	shader->setValue("n_Cam", cam);
	shader->setValue(SVTexture0, child->getFrameBuffer().getAttachment(0));
	shader->setValue(SVTexture1, child->getFrameBuffer().getAttachment(1));
	shader->setValue(SVTexture2, child->getFrameBuffer().getAttachment(2));
	shader->setValue("n_D", child->getFrameBuffer().getDepthAttachment());

	shader->bind();
	GLContext::getContext()->getScreen().draw(getMaterial());
	shader->unbind();




}

}
}

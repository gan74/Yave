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
			"uniform sampler2D n_D;"

			"uniform mat4 n_Inv;"
			"uniform vec3 n_Cam;"
			"uniform vec3 n_Tan;"

			"in vec2 n_TexCoord;"
			"out vec4 n_Out;"

			"vec3 unproj(vec2 C) {"
				"vec4 VP = vec4(vec3(C, texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w;"
			"}"

			+ getBRDFs() +


			"void main() {"
				"vec3 pos = unproj(n_TexCoord);"
				"vec3 view = normalize(n_Cam - pos);"
				"vec4 material = texture(n_2, n_TexCoord);"
				"vec3 normal = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				"float levels = textureQueryLevels(n_Cube);"
				"float roughness = material.x;"

				"const uint samples = 8;"
				"vec3 acc = vec3(0.0);"
				"for(uint i = 0; i != samples; i++) {"
					"vec2 Xi = hammersley(i, samples);"
					"vec2 sphere = Xi * 2.0 * pi;"
					"vec3 D = normalize(brdf_importance_sample(Xi, roughness, normal));"
					"float NoV = dot(D, normal);"
					"if(NoV > 0) {"
						"float w = brdf_cook_torrance(D, view, normal, material);"
						"w += brdf_lambert(D, view, normal, material);"
						"acc += w * textureLod(n_Cube, D, (roughness + NoV) * levels * 0.5).rgb;"
					"}"
				"}"
				"n_Out = vec4(acc / samples, 1.0);"

				//"n_Out = textureLod(n_Cube, reflect(-view, normal), log2(roughness + 1.0) * levels);"
				//"n_Out = vec4(brdf_integrate(n_TexCoord.x, n_TexCoord.y), 0, 1);"
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
	SceneRenderer::FrameData *sceneData = child->getSceneRendererData(ptr);
	math::Matrix4<> invCam = (sceneData->proj * sceneData->view).inverse();
	math::Vec3 cam = sceneData->camera->getPosition();

	const FrameBuffer *fb = GLContext::getContext()->getFrameBuffer();
	child->render(ptr);

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}

	ShaderInstance *shader = getShader();
	shader->setValue("n_Cube", *getCube(), TextureSampler::Trilinear);
	shader->setValue("n_Inv", invCam);;
	shader->setValue("n_Cam", cam);
	shader->setValue(SVTexture1, child->getFrameBuffer().getAttachement(1));
	shader->setValue(SVTexture2, child->getFrameBuffer().getAttachement(2));
	shader->setValue("n_D", child->getFrameBuffer().getDepthAttachement());

	shader->bind();
	GLContext::getContext()->getScreen().draw(getMaterial(), VertexAttribs(), RenderFlag::NoShader);
	shader->unbind();




}

}
}

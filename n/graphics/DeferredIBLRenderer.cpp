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

			"in vec2 n_TexCoord;"
			"out vec4 n_Out;"

			"vec3 unproj(vec2 C) {"
				"vec4 VP = vec4(vec3(C, texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
				"vec4 P = n_Inv * VP;"
				"return P.xyz / P.w;"
			"}"

			+ getBRDFs() +

			"mat3 genWorld(vec3 Z) {"
				"vec3 U = abs(Z.z) > 0.999 ? vec3(1, 0, 0) : vec3(0, 0, 1);"
				"vec3 X = normalize(cross(Z, U));"
				"vec3 Y = cross(X, Z);"
				"return mat3(X, Y, Z);"
			"}"

			"void main() {"
				"vec3 pos = unproj(n_TexCoord);"
				"vec3 V = normalize(n_Cam - pos);"
				"vec4 material = texture(n_2, n_TexCoord);"
				"vec3 N = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				"float levels = textureQueryLevels(n_Cube);"
				"float roughness = saturate(material.x + epsilon);"
				"float F0 = material.z;"
				"float a = sqr(roughness);"

				"const uint samples = 8;"

				"vec3 reflection = reflect(-V, N);"
				"mat3 world = genWorld(reflection);"
				"float NoV = saturate(dot(N, V));"
				"vec3 radiance = vec3(0);"

				"for(uint i = 0; i != samples; i++) {"
					"vec2 Xi = hammersley(i, samples);"
					"vec3 S = brdf_importance_sample(Xi, roughness);"
					"S = normalize(world * S);"
					"vec3 H = normalize(V + S);"
					"float cosT = saturate(dot(S, N));"
					"float sinT = sqrt(1.0 - cosT * cosT);"
					"float NoV = saturate(dot(N, V));"
					"float NoL = saturate(dot(N, S));"
					"float VoH = saturate(dot(V, H));"
					"float F = F_Schlick(VoH, 1.0);"
					"float G = V_Schlick(NoV, NoL, a);"
					"float RoS = saturate(dot(reflection, S));"
					"float bias = pow(1.0 - RoS, 0.25);"
					"radiance += textureLod(n_Cube, S, (roughness + bias) * levels * 0.5).rgb * F * G;"
				"}"
				"n_Out = vec4(radiance / samples, 1.0);"
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

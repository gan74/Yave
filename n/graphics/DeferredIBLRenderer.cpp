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

			"uniform sampler2D n_0;"
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


			"float G_GGX_partial(float VoH, float a2) {"
				"float VoH2 = VoH * VoH;"
				"float tan2 = (1.0 - VoH2 ) / VoH2;"
				"return 2.0 / (1.0 + sqrt(1.0 + a2 * tan2));"
			"}"

			"void main() {"
				"vec3 pos = unproj(n_TexCoord);"
				"vec4 albedo = texture(n_0, n_TexCoord);"
				"vec3 V = normalize(n_Cam - pos);"
				"vec4 material = texture(n_2, n_TexCoord);"
				"vec3 N = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				"float levels = textureQueryLevels(n_Cube);"
				"float roughness = saturate(material.x + epsilon);"
				"float metal = material.y;"
				"vec3 F0 = mix(vec3(material.z), albedo.rgb, metal);"
				"float a = sqr(roughness);"
				"float a2 = sqr(a);"

				"const uint samples = 8;"

				"vec3 reflection = reflect(-V, N);"
				"mat3 world = genWorld(reflection);"
				"float NoV = saturate(dot(N, V));"
				"vec3 radiance = vec3(0);"
				"vec3 Ks = vec3(0);"

				"for(uint i = 0; i != samples; i++) {"
					"vec2 Xi = hammersley(i, samples);"
					"vec3 S = brdf_importance_sample(Xi, roughness);"
					"S = normalize(world * S);"
					"vec3 H = normalize(V + S);"
					"float cosT = saturate(dot(S, N));"
					"float sinT = sqrt(1.0 - cosT * cosT);"
					"float NoV = saturate(dot(N, V));"
					"float NoL = saturate(dot(N, S));"
					"float LoH = saturate(dot(H, S));"
					"float VoH = saturate(dot(V, H));"
					"float NoH = saturate(dot(N, H));"
					"vec3 F = F_Schlick(VoH, F0);"
					"float G = G_GGX_partial(VoH, a2) * G_GGX_partial(LoH,  a2) * sinT / saturate(4.0 * (NoV * NoH + 0.05));"
					//"float G = V_Smith(VoH, LoH, a2);"
					"float RoS = saturate(dot(reflection, S));"
					//"float bias = pow(1.0 - RoS, 0.25);"
					"float bias = roughness;"
					"radiance += textureLod(n_Cube, S, (roughness + bias) * levels * 0.5).rgb * F * G;"
					"Ks += F;"
				"}"
				"vec3 specular = radiance / samples;"
				"Ks = saturate(Ks / samples);"
				"vec3 Kd = (vec3(1.0) - Ks) * (1.0 - metal);"
				"vec3 irradiance = vec3(0);"
				"for(uint i = 0; i != uint(levels); i++) {"
					"irradiance += textureLod(n_Cube, N, i).rgb;"
				"}"
				"irradiance /= vec3(levels * pi);"
				"vec3 diffuse = albedo.rgb * irradiance;"

				//"n_Out = vec4(irradiance, albedo.a);"
				"n_Out = vec4(diffuse * Kd + specular, 1.0);"

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
	shader->setValue(SVTexture0, child->getFrameBuffer().getAttachement(0));
	shader->setValue(SVTexture1, child->getFrameBuffer().getAttachement(1));
	shader->setValue(SVTexture2, child->getFrameBuffer().getAttachement(2));
	shader->setValue("n_D", child->getFrameBuffer().getDepthAttachement());

	shader->bind();
	GLContext::getContext()->getScreen().draw(getMaterial(), VertexAttribs(), RenderFlag::NoShader);
	shader->unbind();




}

}
}

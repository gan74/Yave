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

namespace n {
namespace graphics {

MaterialRenderData getMaterial() {
	MaterialRenderData mat;
	mat.depthTested = false;
	mat.blendMode = BlendMode::Add;
	return mat;
}


CubeMap *getCube() {
	static CubeMap *cube = 0;
	if(!cube) {
		cube = new CubeMap(
			(Image(ImageLoader::load<core::String>("./skybox/top.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/bottom.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/right.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/left.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/front.tga"))),
			(Image(ImageLoader::load<core::String>("./skybox/back.tga"))));

		/*ShaderInstance sh(new Shader<FragmentShader>(
			"layout(location = 0) out vec4 n_0;"
			"layout(location = 1) out vec4 n_1;"
			"layout(location = 2) out vec4 n_2;"
			"layout(location = 3) out vec4 n_3;"
			"layout(location = 4) out vec4 n_4;"
			"layout(location = 5) out vec4 n_5;"

			"void main() {"
				"n_0 = vec4(1, 0, 0, 1);"
				"n_1 = vec4(1, 1, 0, 1);"
				"n_2 = vec4(1, 0, 1, 1);"
				"n_3 = vec4(0, 1, 0, 1);"
				"n_4 = vec4(0, 1, 1, 1);"
				"n_5 = vec4(0, 0, 1, 1);"
			"}"
		), ShaderProgram::NoProjectionShader);
		sh.bind();
		CubeFrameBuffer cbo(1024, ImageFormat::RGBA8);
		cbo.bind();
		GLContext::getContext()->getScreen().draw(getMaterial());
		cube = new CubeMap(cbo.getAttachement());*/
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

			"vec3 unproj(vec2 C, out float d) {"
				"vec4 VP = vec4(vec3(C, d = texture(n_D, C).x) * 2.0 - 1.0, 1.0);"
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


			"float G_Smith_partial(float NoV, float k) {"
				"return NoV / (NoV * (1.0 - k) + k);"
			"}"

			"float G_Smith(float VoH, float LoH, float k) {"
				"return G_Smith_partial(VoH, k) * G_Smith_partial(LoH, k);"
			"}"

			"vec2 integrate(float roughness, float NoV) {"
				"vec3 V = vec3(1.0 - sqr(NoV), 0, NoV);"
				"vec2 I = vec2(0);"

				"const uint samples = 1024;"
				"float a = sqr(roughness);"
				"float k = sqr((roughness + 1.0) * 0.5);"

				"for(uint i = 0; i != samples; i++) {"
					"vec2 Xi = hammersley(i, samples);"
					"vec3 H = brdf_importance_sample(Xi, roughness);"
					"vec3 L = reflect(-V, H);"

					"float NoL = saturate(L.z);"
					"float NoH = saturate(H.z);"
					"float VoH = saturate(dot(V, H));"
					"if(NoL > 0.0) {"
						"float G = G_Smith(NoV, NoL, k);"
						"float vis = G * VoH / (NoH * NoV);"
						"float Fc = pow(1.0 - VoH, 5.0);"
						//"I += vec2(vis, 0.0);"
						"I += vec2(1.0 - Fc, Fc);"
					"}"
				"}"
				"return I / samples;"
			"}"

			"vec3 filterEnv(float roughness, vec3 R) {"
				"vec3 N = R;"
				"vec3 V = R;"

				"mat3 world = genWorld(N);"

				"float sum = 0.0;"
				"vec3 color = vec3(0.0);"

				"const uint samples = 256;"
				"for(uint i = 0; i != samples; i++) {"
					"vec2 Xi = hammersley(i, samples);"
					"vec3 H = normalize(world * brdf_importance_sample(Xi, roughness));"
					"vec3 L = reflect(-V, H);"
					"float NoL = saturate(dot(N, L));"
					"if(NoL > 0.0) {"
						"color += textureLod(n_Cube, L, 0.0).rgb * NoL;"
						"sum += NoL;"
					"}"
				"}"
				"return color / sum;"
			"}"

			"void main() {"
				"float depth = 0;"
				"vec3 pos = unproj(n_TexCoord, depth);"
				"vec4 albedo = texture(n_0, n_TexCoord);"
				"vec4 material = texture(n_2, n_TexCoord);"
				"vec3 N = normalize(texture(n_1, n_TexCoord).xyz * 2.0 - 1.0);"
				"float roughness = saturate(material.x + epsilon);"
				"float metal = material.y * 0.5;"
				"float levels = textureQueryLevels(n_Cube);"

				//"n_Out = vec4(filterEnv(roughness, N), 1.0);"

				"vec3 view = normalize(pos - n_Cam);"
				"if(depth == 1) {"
					"n_Out = textureLod(n_Cube, view, 0);"
				"} else {"
					"vec3 spec = textureLod(n_Cube, reflect(view, N), levels * log2(1 + roughness)).rgb;"
					"vec3 diff = textureLod(n_Cube, N, levels).rgb;"
					"n_Out = vec4((mix(vec3(0.04), albedo.rgb, metal) * spec + albedo.rgb * diff) * 0.5, 1.0);" // bogus
				"}"
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

	const FrameBufferBase *fb = GLContext::getContext()->getFrameBuffer();
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
	GLContext::getContext()->getScreen().draw(getMaterial());
	shader->unbind();




}

}
}

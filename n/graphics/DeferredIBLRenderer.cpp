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
#include "ComputeShaderInstance.h"
#include <n/io/File.h>

namespace n {
namespace graphics {

math::Vec3 importanceSampleGGX(math::Vec2 Xi, float roughness, math::Vec3 N) {
	float a = roughness * roughness;
	float phi = 2 * math::pi * Xi.x();
	float cosTheta = sqrt((1 - Xi.y()) / (1 + (a * a - 1) * Xi.y()));
	float sinTheta = sqrt(1 - cosTheta * cosTheta);
	math::Vec3 H(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

	math::Vec3 upVector = abs(N.z()) < 0.999 ? math::Vec3(0,0,1) : math::Vec3(1,0,0);
	math::Vec3 tangentX = upVector.cross(N).normalized();
	math::Vec3 tangentY = N.cross(tangentX);

	return tangentX * H.x() + tangentY * H.y() + N * H.z();
}

float helper_G_smith(float dotVal, float k) {
	return dotVal / (dotVal * (1 - k) + k);
}

float G_Smith(float dotNV, float dotNL, float roughness) {
	float k = roughness * roughness * 0.5;
	return helper_G_smith(dotNL,k) * helper_G_smith(dotNV,k);
}

math::Vec2 integrateBRDF(float roughness, float NoV) {
	math::Vec3 V;
	V.x() = sqrt(1.0f - NoV * NoV); // sin
	V.y() = 0;
	V.z() = NoV; // cos
	float A = 0;
	float B = 0;
	const uint samples = 1 << 16;
	for(uint i = 0; i < samples; i++) {
		math::Vec2 Xi(random(), random());
		math::Vec3 H = importanceSampleGGX(Xi, roughness, math::Vec3(0, 0, 1));
		math::Vec3 L =  H * V.dot(H) * 2 - V;
		float NoL = std::max(L.z(), 0.f);
		float NoH = std::max(H.z(), 0.f);
		float VoH = std::max(V.dot(H), 0.f);

		if(NoL > 0) {
			float G = G_Smith(NoV, NoL, roughness);
			float G_Vis = G * VoH / (NoH * NoV);
			float Fc = pow(1 - VoH, 5);
			A += (1 - Fc) * G_Vis;
			B += Fc * G_Vis;
		}
	}
	return math::Vec2(A, B) / samples;
}




static MaterialRenderData getMaterial() {
	MaterialRenderData mat;
	mat.depthTested = false;
	mat.depthWrite = false;
	mat.blendMode = BlendMode::Add;
	return mat;
}


static IBLProbe *getProbe() {
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

			"uniform samplerCube cube;"
			"uniform float invRoughnessPower;"
			"uniform uint levels;"
			"uniform sampler2D lut;"


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
					"n_Out = textureLod(cube, view, levels - 1);"
				"} else {"
					"n_Out = iblProbe(cube, invRoughnessPower, levels, reflect(view, gbuffer.normal), gbuffer.roughness);"
					//"n_Out = textureLod(cube, gbuffer.normal, 0);" // <---------
				"}"


				"n_Out = texture(lut, n_TexCoord);"
			"}"
		), ShaderProgram::NoProjectionShader);
	}
	return shader;
}

Texture computeLut() {
	uint size = 256;
	core::Timer timer;
	math::Vec2 *data = new math::Vec2[size * size];
	for(uint x = 0; x != size; x++) {
		for(uint y = 0; y != size; y++) {
			data[x * size + y] = integrateBRDF(x / float(size - 1), (y + 1) / float(size));
		}
	}
	logMsg(core::String("Lut generated in ") + timer.elapsed() + "s", PerfLog);

	io::File file("lut.dat");
	if(!file.open(io::IODevice::Write | io::IODevice::Binary)) {
		fatal("Unable to open lut.dat");
	}
	file.writeBytes(data, sizeof(float) * size * size);
	file.close();

	Texture tex(Image(math::Vec2ui(size), ImageFormat::RG32F, data), false);
	delete[] data;
	return tex;
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




	FrameBuffer::clear(true, true); // <<------------------------------------ REMOVE
	/*if(lut.isNull()) {
		lut = computeLut();
	}*/


	ShaderInstance *shader = getShader();

	shader->setValue("cube", getProbe()->getCubeMap());
	shader->setValue("invRoughnessPower", 1.0 / getProbe()->getRoughnessPower());
	shader->setValue("levels", getProbe()->getLevelCount());
	shader->setValue("lut", lut);

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

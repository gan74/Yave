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

#include "Shader.h"
#include "ShaderProgram.h"
#include <n/core/Timer.h>

namespace n {
namespace graphics {

namespace internal {
	void rebindProgram() {
		ShaderProgram p = GLContext::getContext()->getShaderProgram();
		p.rebind();
	}
}

ShaderBase *ShaderBase::currents[3] = {0};

uint ShaderBase::load(core::String src, uint vers) {
	N_LOG_PERF;
	src = parse(src, vers);
	#ifdef N_SHADER_SRC
	source = src;
	#endif
	handle = gl::createShader(type);
	const char *str = src.toChar();
	gl::shaderSource(handle, 1, &str, 0);
	if(!gl::compileShader(handle)) {
		throw ShaderCompilationException(gl::getShaderInfoLog(handle));
	}
	return vers;
}

core::String ShaderBase::parse(core::String src, uint vers) {
	core::String libs[] = {
			"vec4 n_gbuffer0(vec4 color, vec3 normal, float roughness, float metal) {"
				"return color;"
			"}"
			"vec4 n_gbuffer1(vec4 color, vec3 normal, float roughness, float metal) {"
				"return vec4(normalize(normal).xyz * 0.5 + 0.5, 0.0);"
			"}"
			"vec4 n_gbuffer2(vec4 color, vec3 normal, float roughness, float metal) {"
				"return vec4(roughness, metal, 0, 1.0);"
			"}"
			"vec4 n_gbuffer2(vec4 color, vec3 normal, float roughness, float metal, float sun) {"
				"return vec4(roughness, metal, 0, sun);"
			"}"
			"flat in uint n_InstanceID;"
			"\n#define n_BufferIndex n_InstanceID\n",

			"layout(location = 4) in uint n_DrawID;"
			"flat out uint n_InstanceID;"
			"\n#define n_BufferIndex (n_InstanceID = n_DrawID)\n",

			"flat out uint n_InstanceID;"
	};
	uint bufferSize = UniformBuffer<math::Matrix4<>>::getMaxSize();
	core::String ver = core::String("#version ") + vers + "\n#extension GL_ARB_bindless_texture : enable \n";
	core::String model = "\n #define n_ModelMatrix n_ModelMatrices[n_BufferIndex] \n"
						 "uniform n_ModelMatrixBuffer { mat4 n_ModelMatrices[" + core::String(bufferSize) + "]; };";
	core::String material = "layout(std140) uniform n_MaterialBuffer { n_MaterialType n_Materials[" + core::String(bufferSize) + "]; };"
							"\n #define n_Material n_Materials[n_BufferIndex] \n";
	core::String common = "layout(std140, row_major) uniform; "
						  "layout (std140, row_major) buffer; "
						  "const float pi = " + core::String(math::pi) + "; "
						  "const float epsilon = 0.001; "
						  "float sqr(float x) { return x * x; }  "
						  "float saturate(float x) { return clamp(x, 0.0, 1.0); }"
						  "vec2 saturate(vec2 x) { return clamp(x, vec2(0.0), vec2(1.0)); }"
						  "vec3 saturate(vec3 x) { return clamp(x, vec3(0.0), vec3(1.0)); }"
						  "vec4 saturate(vec4 x) { return clamp(x, vec4(0.0), vec4(1.0)); }"
						  "vec2 sphereMap(vec3 U, vec3 N) { vec3 R = reflect(U, N); float m = -2.0 * sqrt(sqr(R.x) + sqr(R.y + 1.0) + sqr(R.z)); return R.xz / m + 0.5; }"
						  "vec3 hemisphereSample(vec2 uv) {float phi = uv.y * 2.0 * pi; float cosTheta = 1.0 - uv.x; float sinTheta = sqrt(1.0 - cosTheta * cosTheta); return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta); }"
						  "vec2 hammersley(uint i, uint N) { "
							  "uint bits = i;"
							  "bits = (bits << 16u) | (bits >> 16u);"
							  "bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);"
							  "bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);"
							  "bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);"
							  "bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);"
							  "float VdC = float(bits) * 2.3283064365386963e-10;"
							  "return vec2(float(i) / float(N), VdC);"
						  "}"
						  "uniform uint n_BaseInstance;"
						  "struct n_MaterialType { "
							  "vec4 color; "
							  "float metallic; "
							  "float diffuseIntensity; "
							  "float normalIntensity; "
							  "float roughnessIntensity; "
							  "sampler2D diffuse; "
							  "sampler2D normal; "
							  "sampler2D roughness; "
						  "};";
	uint vit = src.find("#version");
	if(vit != uint(-1)) {
		uint l = src.find("\n", vit);
		if(l != uint(-1)) {
			bool ok = true;
			uint v = src.subString(vit + 9).get<uint>([&]() { ok = false; });
			if(ok && v) {
				vers = v;
			} else {
				logs += "Unable to parse #version.\n";
			}
		}
		src.replace("#version", "//#version");
	}
	return ver + "\n" +  common + "\n" + libs[type] + "\n" + src.replaced("N_DECLARE_MODEL_MATRIX", model).replaced("N_DECLARE_MATERIAL_BUFFER", material);
}


ShaderBase::ShaderBase(ShaderType t) : type(t), version(0), handle(0) {
}

ShaderBase::~ShaderBase() {
	if(handle) {
		gl::Handle h = handle;
		GLContext::getContext()->addGLTask([=]() { gl::deleteShader(h); });
	}
}

const core::String &ShaderBase::getLogs() const {
	return logs;
}

bool ShaderBase::isValid() const {
	return handle && version;
}

uint ShaderBase::getVersion() const {
	return version;
}

bool ShaderBase::isCurrent() const {
	return currents[type] == this;
}

ShaderType ShaderBase::getType() const {
	return type;
}


}
}

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

#include <iostream>

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
				"return vec4(roughness, metal, 0.04, 0);"
			"}",
			"",
			""
	};
	core::String ver = core::String("#version ") + vers + "\n";
	core::String model = "\n#define n_ModelMatrix n_ModelMatrices[n_BaseInstance + gl_InstanceID] \n"
						 "uniform n_ModelMatrixBuffer { mat4 n_ModelMatrices[" + core::String(UniformBuffer<math::Matrix4<>>::getMaxSize()) + "]; }; uniform uint n_BaseInstance;\n";
	core::String common = "layout(std140, row_major) uniform; layout (std140, row_major) buffer; const float pi = " + core::String(math::pi) + "; float sqr(float x) { return x * x; }  float saturate(float x) { return clamp(x, 0.0, 1.0); }" +
	"vec2 sphereMap(vec3 U, vec3 N) {"
		"vec3 R = reflect(U, N);"
		"float m = -2.0 * sqrt(sqr(R.x) + sqr(R.y + 1.0) + sqr(R.z));"
		"return R.xz / m + 0.5;"
	"}";
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
	return ver + common + libs[type] + "\n" + (type == VertexShader ? src.replaced("N_DECLARE_MODEL_MATRIX", model) : src);
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

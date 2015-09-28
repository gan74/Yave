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

	gl::GLenum glType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER};

	const char *data = src.toChar();
	handle = gl::glCreateShaderProgramv(glType[type], 1, &data);
	int res = 0;
	gl::glGetProgramiv(handle, GL_LINK_STATUS, &res);
	if(!res || !handle) {
		int size = 0;
		gl::glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &size);
		char *msg = new char[size + 1];
		gl::glGetProgramInfoLog(handle, size, &res, msg);
		gl::glDeleteProgram(handle);
		handle = 0;
		msg[size] = '\0';
		logs = msg;
		delete[] msg;
		throw ShaderCompilationException(logs);
	}
	getUniforms();
	return vers;
}

static core::String getDefines() {
	uint max = GLContext::getContext()->getHWInt(GLContext::MaxVaryings);
	core::String varyings[] = {"n_Position",
							   "n_ScreenPosition",
							   "n_Normal",
							   "n_Tangent",
							   "n_Binormal",
							   "n_View",
							   "n_TexCoord"};

	core::String defs;
	for(uint i = 0; i != sizeof(varyings) / sizeof(varyings[0]); i++) {
		defs += "\n#define " + varyings[i] + "Location " + (max - i - 1) + "\n";
	}
	return defs;
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
	core::String common = "const float pi = " + core::String(math::pi) + "; float sqr(float x) { return x * x; }  float saturate(float x) { return clamp(x, 0.0, 1.0); }";
	if((type == VertexShader || type == GeometryShader) && src.find("gl_PerVertex") == uint(-1)) {
		common += "out gl_PerVertex { vec4 gl_Position; };";
	}
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
	return ver + getDefines() + common + libs[type] + src;
}

void ShaderBase::getUniforms() {
	const uint max = 512;
	char name[max];
	int uniforms = 0;
	gl::glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);
	for(uint i = 0; i != uint(uniforms); i++) {
		gl::GLsizei size = 0;
		gl::GLenum type = GL_NONE;
		gl::glGetActiveUniform(handle, i, max - 1, 0, &size, &type, name);
		core::String uniform = name;
		if(uniform.endsWith("[0]")) {
			uniform = uniform.subString(0, uniform.size() - 3);
		}
		UniformInfo info({gl::glGetUniformLocation(handle, name), (uint)size});
		if(isSampler(type)) {
			uint slot = samplerCount++;
			setValue(info.addr, int(slot));
			info.addr = slot;
		}
		uniformsInfo[uniform] = info;
	}
	bindings = new internal::TextureBinding[samplerCount];
}

ShaderBase::ShaderBase(ShaderType t) : type(t), version(0), handle(0), samplerCount(0), bindings(0) {
}

ShaderBase::~ShaderBase() {
	if(handle) {
		gl::GLuint h = handle;
		GLContext::getContext()->addGLTask([=]() { gl::glDeleteProgram(h); });
	}
}

void ShaderBase::bindStandards() {
	setValue("n_ProjectionMatrix", GLContext::getContext()->getProjectionMatrix());
	setValue("n_ViewMatrix", GLContext::getContext()->getViewMatrix());
	setValue("n_ViewportSize", math::Vec2(GLContext::getContext()->getViewport()));
	setValue("n_ModelMatrix", GLContext::getContext()->getModelMatrix());
	setValue("n_ViewProjectionMatrix", GLContext::getContext()->getProjectionMatrix() * GLContext::getContext()->getViewMatrix());
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

ShaderBase::UniformAddr ShaderBase::getAddr(const core::String &name) const {
	return getInfo(name).addr;
}

bool ShaderBase::isCurrent() const {
	return currents[type] == this;
}

ShaderType ShaderBase::getType() const {
	return type;
}


void ShaderBase::setValue(UniformAddr addr, int a) const {
	gl::glProgramUniform1i(handle, addr, a);
}

void ShaderBase::setValue(UniformAddr addr, uint a) const {
	gl::glProgramUniform1ui(handle, addr, a);
}

void ShaderBase::setValue(UniformAddr addr, float f) const {
	gl::glProgramUniform1f(handle, addr, f);
}

void ShaderBase::setValue(UniformAddr addr, double f) const {
	gl::glProgramUniform1f(handle, addr, f);
}

void ShaderBase::setValue(UniformAddr addr, const math::Vec2i &v) const {
	gl::glProgramUniform2iv(handle, addr, 1, v.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Vec3i &v) const {
	gl::glProgramUniform3iv(handle, addr, 1, v.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Vec2 &v) const {
	gl::glProgramUniform2fv(handle, addr, 1, v.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Vec3 &v) const {
	gl::glProgramUniform3fv(handle, addr, 1, v.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Vec4 &v) const {
	gl::glProgramUniform4fv(handle, addr, 1, v.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Matrix2<float> &m) const {
	gl::glProgramUniformMatrix2fv(handle, addr, 1, GL_TRUE, m.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Matrix3<float> &m) const {
	gl::glProgramUniformMatrix3fv(handle, addr, 1, GL_TRUE, m.begin());
}

void ShaderBase::setValue(UniformAddr addr, const math::Matrix4<float> &m) const {
	gl::glProgramUniformMatrix4fv(handle, addr, 1, GL_TRUE, m.begin());
}

void ShaderBase::setValue(UniformAddr slot, const Texture &t, TextureSampler sampler) const {
	if(slot != UniformAddr(GL_INVALID_INDEX)) {
		bindings[slot] = t;
		bindings[slot] = sampler;
		if(isCurrent()) {
			bindings[slot].bind(slot);
		} else {
			t.prepare();
		}
	}
}

ShaderBase::UniformInfo ShaderBase::getInfo(const core::String &name) const {
	return uniformsInfo.get(name, UniformInfo{UniformAddr(GL_INVALID_INDEX), 0});
}

}
}

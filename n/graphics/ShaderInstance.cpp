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

#include "ShaderInstance.h"
#include "GLContext.h"


namespace n {
namespace graphics {


ShaderInstance *ShaderInstance::current = 0;


ShaderInstance::ShaderInstance(Shader<FragmentShader> *frag, Shader<VertexShader> *vert, Shader<GeometryShader> *geom) : handle(0), samplerCount(0), bases{frag, vert, geom} {
	compile();
}

ShaderInstance::ShaderInstance(Shader<FragmentShader> *frag, ShaderProgram::StandardVertexShader vert, Shader<GeometryShader> *geom) : ShaderInstance(frag, ShaderProgram::getStandardVertexShader(vert), geom) {

}

ShaderInstance::~ShaderInstance() {
	if(handle) {
		gl::GLuint h = handle;
		GLContext::getContext()->addGLTask([=]() { gl::glDeleteProgram(h); });
	}
}

const ShaderInstance *ShaderInstance::getCurrent() {
	return current;
}

void ShaderInstance::bind() {
	GLContext::getContext()->program = 0;
	if(current != this) {
		rebind();
	}
}

void ShaderInstance::rebind() {
	GLContext::getContext()->program = 0;
	gl::glUseProgram(handle);
	current = this;
	//bindStandards();
}

void ShaderInstance::unbind() {
	fatal("ll");
}

void ShaderInstance::compile() {
	handle = gl::glCreateProgram();
	bool val = true;
	for(uint i = 0; i != 3; i++) {
		if(bases[i]) {
			gl::glAttachShader(handle, bases[i]->handle);
			val &= bases[i]->isValid();
		}
	}
	gl::glLinkProgram(handle);
	int res = 0;
	gl::glGetProgramiv(handle, GL_LINK_STATUS, &res);
	if(!res || !val) {
		int size = 0;
		gl::glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &size);
		char *msg = new char[size + 1];
		gl::glGetProgramInfoLog(handle, size, &res, msg);
		gl::glDeleteProgram(handle);
		handle = 0;
		msg[size] = '\0';
		core::String logs = msg;
		delete[] msg;
		throw ShaderLinkingException(logs);
	} else {
		getUniforms();
	}
}

void ShaderInstance::getUniforms() {
	const uint max = 512;
	char name[max];
	int uniforms = 0;
	gl::glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);
	for(uint i = 0; i != SVMax; i++) {
		standards[i] = UniformAddr(GL_INVALID_INDEX);
	}
	for(uint i = 0; i != (uint)uniforms; i++) {
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
		int std = computeStandardIndex(uniform);
		if(std != UniformAddr(GL_INVALID_INDEX)) {
			standards[std] = info.addr;
		}
	}
	bindings = new internal::TextureBinding[samplerCount];
}

ShaderInstance::UniformAddr ShaderInstance::computeStandardIndex(const core::String &name) {
	for(uint i = 0; i != SVMax; i++) {
		if(name == ShaderValueName[i]) {
			return i;
		}
	}
	return UniformAddr(GL_INVALID_INDEX);
}

void ShaderInstance::bindStandards() const {
	setValue("n_ProjectionMatrix", GLContext::getContext()->getProjectionMatrix());
	setValue("n_ViewMatrix", GLContext::getContext()->getViewMatrix());
	setValue("n_ViewportSize", math::Vec2(GLContext::getContext()->getViewport()));
	setValue("n_ModelMatrix", GLContext::getContext()->getModelMatrix());
	setValue("n_ViewProjectionMatrix", GLContext::getContext()->getProjectionMatrix() * GLContext::getContext()->getViewMatrix());
}

void ShaderInstance::setValue(UniformAddr addr, int a) const {
	gl::glProgramUniform1i(handle, addr, a);
}

void ShaderInstance::setValue(UniformAddr addr, uint a) const {
	gl::glProgramUniform1ui(handle, addr, a);
}

void ShaderInstance::setValue(UniformAddr addr, float f) const {
	gl::glProgramUniform1f(handle, addr, f);
}

void ShaderInstance::setValue(UniformAddr addr, double f) const {
	gl::glProgramUniform1f(handle, addr, f);
}

void ShaderInstance::setValue(UniformAddr addr, const math::Vec2i &v) const {
	gl::glProgramUniform2iv(handle, addr, 1, v.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Vec3i &v) const {
	gl::glProgramUniform3iv(handle, addr, 1, v.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Vec2 &v) const {
	gl::glProgramUniform2fv(handle, addr, 1, v.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Vec3 &v) const {
	gl::glProgramUniform3fv(handle, addr, 1, v.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Vec4 &v) const {
	gl::glProgramUniform4fv(handle, addr, 1, v.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Matrix2<float> &m) const {
	gl::glProgramUniformMatrix2fv(handle, addr, 1, GL_TRUE, m.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Matrix3<float> &m) const {
	gl::glProgramUniformMatrix3fv(handle, addr, 1, GL_TRUE, m.begin());
}

void ShaderInstance::setValue(UniformAddr addr, const math::Matrix4<float> &m) const {
	gl::glProgramUniformMatrix4fv(handle, addr, 1, GL_TRUE, m.begin());
}

void ShaderInstance::setValue(UniformAddr slot, const Texture &t, TextureSampler sampler) const {
	if(slot != UniformAddr(GL_INVALID_INDEX)) {
		bindings[slot] = t;
		bindings[slot] = sampler;
		if(current == this) {
			bindings[slot].bind(slot);
		} else {
			t.prepare();
		}
	}
}

ShaderInstance::UniformAddr ShaderInstance::getAddr(const core::String &name) const {
	return getInfo(name).addr;
}

ShaderInstance::UniformInfo ShaderInstance::getInfo(const core::String &name) const {	return uniformsInfo.get(name, UniformInfo{UniformAddr(GL_INVALID_INDEX), 0});
}

}
}

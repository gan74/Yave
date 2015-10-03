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

const char *ShaderValueName[ShaderValue::SVMax] = {
	"n_Color",
	"n_Metallic",
	"n_DiffuseMap",
	"n_DiffuseMul",
	"n_NormalMap",
	"n_NormalMul",
	"n_RoughnessMap",
	"n_RoughnessMul",
	"n_0",
	"n_1",
	"n_2",
	"n_3"
};


ShaderInstance *ShaderInstance::current = 0;

ShaderInstance::ShaderInstance(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom) : handle(0), samplerCount(0), bases{frag, vert, geom} {
	compile();
}

ShaderInstance::ShaderInstance(const Shader<FragmentShader> *frag, ShaderProgram::StandardVertexShader vert, const Shader<GeometryShader> *geom) : ShaderInstance(frag, ShaderProgram::getStandardVertexShader(vert), geom) {
}

ShaderInstance::~ShaderInstance() {
	if(handle) {
		gl::GLuint h = handle;
		GLContext::getContext()->addGLTask([=]() { gl::glDeleteProgram(h); });
	}
	delete[] texBindings;
	delete[] bufferBindings;
}

const ShaderInstance *ShaderInstance::getCurrent() {
	return current;
}

void ShaderInstance::validateState() {
	current->bindStandards();
	current->bindTextures();
	current->bindBuffers();
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
}

void ShaderInstance::unbind() {
	current = 0;
	internal::rebindProgram();
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
	}
	getUniforms();
}

void ShaderInstance::getUniforms() {
	const uint max = 1024;
	char name[max];
	int uniforms = 0;
	gl::glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);
	for(uint i = 0; i != SVMax; i++) {
		standards[i] = UniformInfo{UniformAddr(GL_INVALID_INDEX), 0, gl::GLenum(GL_NONE)};
	}
	for(uint i = 0; i != uint(uniforms); i++) {
		gl::GLsizei size = 0;
		gl::GLenum type = GL_NONE;
		gl::glGetActiveUniform(handle, i, max - 1, 0, &size, &type, name);
		core::String uniform = name;
		if(uniform.contains(".")) {
			continue;
		}
		if(uniform.endsWith("[0]")) {
			uniform = uniform.subString(0, uniform.size() - 3);
		}
		UniformInfo info({gl::glGetUniformLocation(handle, name), (uint)size, type});
		if(isSampler(type)) {
			uint slot = samplerCount++;
			setValue(info, int(slot));
			info.addr = slot;
		}
		uniformsInfo[uniform] = info;
		int std = computeStandardIndex(uniform);
		if(std != UniformAddr(GL_INVALID_INDEX)) {
			standards[std] = info;
		}
	}
	texBindings = new internal::TextureBinding[samplerCount];

	UniformAddr *texAddr = new UniformAddr[samplerCount];
	for(uint i = 0; i != samplerCount; i++) {
		texAddr[i] = i;
	}
	setValue("n_Textures", texAddr, samplerCount);
	delete[] texAddr;

	gl::glGetProgramiv(handle, GL_ACTIVE_UNIFORM_BLOCKS, &uniforms);
	bufferBindings = new core::SmartPtr<DynamicBufferBase::Data>[uniforms];
	for(uint i = 0; i != uint(uniforms); i++) {
		int len = 0;
		gl::glGetActiveUniformBlockName(handle, i, max, &len, name);
		uint index = gl::glGetUniformBlockIndex(handle, name);
		gl::glUniformBlockBinding(handle, index, buffers.size());
		bufferBindings[i] = 0;
		buffers.insert(core::String(name, len), index);
	}
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
	setValue("n_ModelMatrix", GLContext::getContext()->getModelMatrix());

	setValue("n_ProjectionMatrix", GLContext::getContext()->getProjectionMatrix());
	setValue("n_ViewMatrix", GLContext::getContext()->getViewMatrix());
	setValue("n_ViewportSize", math::Vec2(GLContext::getContext()->getViewport()));
	setValue("n_ViewProjectionMatrix", GLContext::getContext()->getProjectionMatrix() * GLContext::getContext()->getViewMatrix());
}

void ShaderInstance::bindTextures() const {
	for(uint i = 0; i != samplerCount; i++) {
		texBindings[i].bind(i);
	}
}

void ShaderInstance::bindBuffers() const {
	for(uint i = 0; i != buffers.size(); i++) {
		if(bufferBindings[i]) {
			bufferBindings[i]->update();
			gl::glBindBufferBase(GL_UNIFORM_BUFFER, i, bufferBindings[i]->handle);
		}
	}
}

void ShaderInstance::setValue(UniformInfo info, const int *a, uint count) const {
	gl::glProgramUniform1iv(handle, info.addr, count, a);
}

void ShaderInstance::setValue(UniformInfo info, const uint *a, uint count) const {
	gl::glProgramUniform1uiv(handle, info.addr, count, a);
}

void ShaderInstance::setValue(UniformInfo info, const float *f, uint count) const {
	gl::glProgramUniform1fv(handle, info.addr, count, f);
}

void ShaderInstance::setValue(UniformInfo info, const math::Vec2i *v, uint count) const {
	gl::glProgramUniform2iv(handle, info.addr, count, v->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Vec3i *v, uint count) const {
	gl::glProgramUniform3iv(handle, info.addr, count, v->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Vec2 *v, uint count) const {
	gl::glProgramUniform2fv(handle, info.addr, count, v->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Vec3 *v, uint count) const {
	gl::glProgramUniform3fv(handle, info.addr, count, v->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Vec4 *v, uint count) const {
	gl::glProgramUniform4fv(handle, info.addr, count, v->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Matrix2<float> *m, uint count) const {
	gl::glProgramUniformMatrix2fv(handle, info.addr, count, GL_TRUE, m->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Matrix3<float> *m, uint count) const {
	gl::glProgramUniformMatrix3fv(handle, info.addr, count, GL_TRUE, m->begin());
}

void ShaderInstance::setValue(UniformInfo info, const math::Matrix4<float> *m, uint count) const {
	gl::glProgramUniformMatrix4fv(handle, info.addr, count, GL_TRUE, m->begin());
}

void ShaderInstance::setValue(UniformInfo info, const Texture &t, TextureSampler sampler) const {
	if(info.type == GL_UNSIGNED_INT_VEC2 && GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
		t.prepare();
		gl::glProgramUniformHandleui64ARB(handle, info.addr, t.getBindlessId());
	} else {
		UniformAddr slot = info.addr;
		if(slot != UniformAddr(GL_INVALID_INDEX)) {
			texBindings[slot] = t;
			texBindings[slot] = sampler;
		}
	}
}

ShaderInstance::UniformInfo ShaderInstance::getInfo(const core::String &name) const {
	return uniformsInfo.get(name, UniformInfo{UniformAddr(GL_INVALID_INDEX), 0, gl::GLenum(GL_NONE)});
}

}
}

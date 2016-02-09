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

#include "ShaderInstanceBase.h"
#include "GLContext.h"
#include <n/core/Timer.h>

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
	"n_3",
	"n_Uniforms",
	"n_ModelMatrixBuffer",
	"n_BaseInstance",
	"n_ProjectionMatrix",
	"n_ViewMatrix",
	"n_ViewportSize",
	"n_ViewProjectionMatrix"
};

ShaderInstanceBase *ShaderInstanceBase::current = 0;

uint maxTextures() {
	return GLContext::getContext()->getHWInt(GLContext::MaxTextures);
}

struct BindingData
{
	BindingData() : handle(0), type(TextureType::Texture2D), sampler(0) {
	}

	gl::Handle handle;
	TextureType type;
	gl::Handle sampler;
};







ShaderInstanceBase::TextureBinding::TextureBinding() : sampler(TextureSampler::MaxSamplers) {
}

void ShaderInstanceBase::TextureBinding::bind(uint slot) const {
	static gl::Handle samplers[TextureSampler::MaxSamplers][2] = {{0}, {0}};
	TextureSampler smp = sampler;
	BindingData t;
	if(tex) {
		t.handle = tex->handle;
		t.type = tex->type;
		if(!samplers[smp][tex->hasMips]) {
			samplers[smp][tex->hasMips] = gl::createSampler(smp, tex->hasMips);
		}
		t.sampler = samplers[smp][tex->hasMips];
	}
	gl::bindTextureUnit(slot, t.type, t.handle);
	gl::bindSampler(slot, t.sampler);
}

ShaderInstanceBase::ImageBinding::ImageBinding() : access(TextureAccess::ReadWrite), format(ImageFormat::RGBA8) {
}

void ShaderInstanceBase::ImageBinding::bind(uint slot) const {
	if(tex) {
		gl::bindTextureImage(slot, tex->handle, access, gl::getTextureFormat(format));
	}
}








ShaderInstanceBase::ShaderInstanceBase() : handle(0), samplerCount(0), imageCount(0) {
}

ShaderInstanceBase::~ShaderInstanceBase() {
	if(handle) {
		gl::Handle h = handle;
		GLContext::getContext()->addGLTask([=]() { gl::deleteProgram(h); });
	}
	delete[] texBindings;
	delete[] imBindings;
	delete[] bufferBindings;
}

const ShaderInstanceBase *ShaderInstanceBase::getCurrent() {
	return current;
}

void ShaderInstanceBase::validateState() {
	current->bindStandards();
	current->bindTextures();
	current->bindBuffers();
}

void ShaderInstanceBase::bind() {
	GLContext::getContext()->program = 0;
	if(current != this) {
		rebind();
	}
}

void ShaderInstanceBase::rebind() {
	GLContext::getContext()->program = 0;
	gl::useProgram(handle);
	current = this;
}

void ShaderInstanceBase::unbind() {
	current = 0;
	internal::rebindProgram();
}

void ShaderInstanceBase::compile(const ShaderBase **bases, uint count) {
	if(isCompiled()) {
		gl::deleteProgram(handle);
	}
	handle = gl::createProgram();
	bool val = true;
	for(uint i = 0; i != count; i++) {
		if(bases[i]) {
			bases[i]->compile();
			gl::attachShader(handle, bases[i]->handle);
			val &= bases[i]->isValid();
		}
	}
	if(!gl::linkProgram(handle) || !val) {
		throw ShaderLinkingException(gl::getProgramInfoLog(handle));
	}
	getUniforms();
}

bool ShaderInstanceBase::isCompiled() const {
	return handle;
}

void ShaderInstanceBase::getUniforms() {
	uint uniforms = gl::getProgramInt(handle, gl::ActiveUniforms);
	for(uint i = 0; i != SVMax; i++) {
		standards[i] = UniformInfo{gl::InvalidIndex, 0, gl::InvalidType};
	}
	for(uint i = 0; i != uint(uniforms); i++) {
		uint size = 0;
		gl::UniformType type;
		core::String name = gl::getActiveUniformInfo(handle, i, &size, &type);
		core::String uniform = name;
		if(uniform.contains(".")) {
			continue;
		}
		if(uniform.endsWith("[0]")) {
			uniform = uniform.subString(0, uniform.size() - 3);
		}
		UniformInfo info({gl::getUniformLocation(handle, name), (uint)size, type});
		// textures
		if(gl::isSamplerType(type)) {
			uint slot = samplerCount++;
			setValue(info, int(slot));
			info.addr = slot;
		}
		// images
		if(gl::isImageType(type)) {
			uint slot = imageCount++;
			setValue(info, int(slot));
			info.addr = slot;
		}

		uniformsInfo[uniform] = info;
		int std = computeStandardIndex(uniform);
		if(std != gl::InvalidIndex) {
			standards[std] = info;
		}
	}
	texBindings = new TextureBinding[samplerCount];
	imBindings = new ImageBinding[imageCount];

	UniformAddr *texAddr = new UniformAddr[samplerCount];
	for(uint i = 0; i != samplerCount; i++) {
		texAddr[i] = i;
	}
	setValue("n_Textures", texAddr, samplerCount);
	delete[] texAddr;

	uint bufferCount = gl::getProgramInt(handle, gl::ActiveBuffers);
	uniforms = gl::getProgramInt(handle, gl::ActiveBlocks);
	bufferBindings = new core::SmartPtr<DynamicBufferBase::Data>[uniforms + bufferCount];

	for(uint i = 0; i != uniforms; i++) {
		core::String name = gl::getActiveUniformBlockName(handle, i);
		gl::setUniformBlockBinding(handle, name, i);
		bufferBindings[i] = 0;
		buffers.insert(name, i);

		int std = computeStandardIndex(name);
		if(std != gl::InvalidIndex) {
			standards[std] = UniformInfo({int(i), 0, 0});
		}
	}

	for(uint j = 0; j != bufferCount; j++) {
		core::String name = gl::getActiveBufferName(handle, j);
		uint i = j + uniforms;
		gl::setStorageBlockBinding(handle, name, i);
		bufferBindings[i] = 0;
		buffers.insert(name, i);

		int std = computeStandardIndex(name);
		if(std != gl::InvalidIndex) {
			standards[std] = UniformInfo({int(i), 0, 0});
		}
	}

}

ShaderInstanceBase::UniformAddr ShaderInstanceBase::computeStandardIndex(const core::String &name) {
	for(uint i = 0; i != SVMax; i++) {
		if(name == ShaderValueName[i]) {
			return i;
		}
	}
	return gl::InvalidIndex;
}

void ShaderInstanceBase::bindStandards() const {
	static core::Timer timer;

	setBuffer(SVModelMatrixBuffer, GLContext::getContext()->models);

	setValue("n_Time", timer.elapsed());
	setValue(SVProjMatrix, GLContext::getContext()->getProjectionMatrix());
	setValue(SVViewMatrix, GLContext::getContext()->getViewMatrix());
	setValue(SVViewportSize, math::Vec2(GLContext::getContext()->getViewport()));
	setValue(SVViewProjMatrix, GLContext::getContext()->getProjectionMatrix() * GLContext::getContext()->getViewMatrix());
}

void ShaderInstanceBase::bindTextures() const {
	for(uint i = 0; i != samplerCount; i++) {
		texBindings[i].bind(i);
	}
	for(uint i = 0; i != imageCount; i++) {
		imBindings[i].bind(i);
	}
}

void ShaderInstanceBase::bindBuffers() const {
	for(uint i = 0; i != buffers.size(); i++) {
		if(bufferBindings[i]) {
			bufferBindings[i]->update();
			gl::bindBufferBase(bufferBindings[i]->type, i, bufferBindings[i]->handle);
		}
	}
}

void ShaderInstanceBase::setValue(UniformInfo info, const int *a, uint count) const {
	gl::programUniform1iv(handle, info.addr, count, a);
}

void ShaderInstanceBase::setValue(UniformInfo info, const uint *a, uint count) const {
	gl::programUniform1uiv(handle, info.addr, count, a);
}

void ShaderInstanceBase::setValue(UniformInfo info, const float *f, uint count) const {
	gl::programUniform1fv(handle, info.addr, count, f);
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Vec2i *v, uint count) const {
	gl::programUniform2iv(handle, info.addr, count, v->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Vec3i *v, uint count) const {
	gl::programUniform3iv(handle, info.addr, count, v->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Vec2 *v, uint count) const {
	gl::programUniform2fv(handle, info.addr, count, v->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Vec3 *v, uint count) const {
	gl::programUniform3fv(handle, info.addr, count, v->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Vec4 *v, uint count) const {
	gl::programUniform4fv(handle, info.addr, count, v->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Matrix2<float> *m, uint count) const {
	gl::programUniformMatrix2fv(handle, info.addr, count, true, m->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Matrix3<float> *m, uint count) const {
	gl::programUniformMatrix3fv(handle, info.addr, count, true, m->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const math::Matrix4<float> *m, uint count) const {
	gl::programUniformMatrix4fv(handle, info.addr, count, true, m->begin());
}

void ShaderInstanceBase::setValue(UniformInfo info, const Texture &t, TextureSampler sampler) const {
	/*if(gl::isBindlessHandle(info.type) && GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
		t.synchronize();
		gl::programUniformHandleui64(handle, info.addr, t.getBindlessId());
	} else*/ {
		UniformAddr slot = info.addr;
		if(slot != UniformAddr(gl::InvalidIndex)) {
			texBindings[slot] = t;
			texBindings[slot] = sampler;
		}
	}
}

void ShaderInstanceBase::setValue(UniformInfo info, const CubeMap &t, TextureSampler sampler) const {
	/*if(gl::isBindlessHandle(info.type) && GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
		t.synchronize();
		gl::programUniformHandleui64(handle, info.addr, t.getBindlessId());
	} else*/ {
		UniformAddr slot = info.addr;
		if(slot != UniformAddr(gl::InvalidIndex)) {
			texBindings[slot] = t;
			texBindings[slot] = sampler;
		}
	}
}

void ShaderInstanceBase::setValue(UniformInfo info, const Texture &t, TextureAccess access) const {
	UniformAddr slot = info.addr;
	if(slot != UniformAddr(gl::InvalidIndex)) {
		imBindings[slot] = t;
		imBindings[slot] = access;
	}
}

void ShaderInstanceBase::setBuffer(const core::String &name, const DynamicBufferBase &buffer) const {
	int slot = buffers.get(name, gl::InvalidIndex);
	if(slot != gl::InvalidIndex) {
		bufferBindings[slot] = buffer.data;
	}
}

void ShaderInstanceBase::setBuffer(ShaderValue v, const DynamicBufferBase &buffer) const {
	if(standards[v].addr != gl::InvalidIndex) {
		bufferBindings[standards[v].addr] = buffer.data;
	}
}

ShaderInstanceBase::UniformInfo ShaderInstanceBase::getInfo(const core::String &name) const {
	return uniformsInfo.get(name, UniformInfo{UniformAddr(gl::InvalidIndex), 0, gl::UniformType()});
}

ShaderInstanceBase::UniformInfo ShaderInstanceBase::getInfo(ShaderValue v) const {
	return standards[v];
}

}
}

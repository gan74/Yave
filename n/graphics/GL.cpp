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


#include "GL.h"
#include <n/core/Map.h>
#include <n/core/Timer.h>
#include <GL/glew.h>

#include <iostream>

namespace n {
namespace graphics {
namespace gl {

bool isSamplerType(UniformType type) {
	static constexpr GLenum samplers[] = {
	GL_SAMPLER_1D,
	GL_SAMPLER_2D,
	GL_SAMPLER_3D,
	GL_SAMPLER_CUBE,
	GL_SAMPLER_1D_SHADOW,
	GL_SAMPLER_2D_SHADOW,
	GL_SAMPLER_1D_ARRAY,
	GL_SAMPLER_2D_ARRAY,
	GL_SAMPLER_1D_ARRAY_SHADOW,
	GL_SAMPLER_2D_ARRAY_SHADOW,
	GL_SAMPLER_2D_MULTISAMPLE,
	GL_SAMPLER_2D_MULTISAMPLE_ARRAY,
	GL_SAMPLER_CUBE_SHADOW,
	GL_SAMPLER_BUFFER,
	GL_SAMPLER_2D_RECT,
	GL_SAMPLER_2D_RECT_SHADOW,
	GL_INT_SAMPLER_1D,
	GL_INT_SAMPLER_2D,
	GL_INT_SAMPLER_3D,
	GL_INT_SAMPLER_CUBE,
	GL_INT_SAMPLER_1D_ARRAY,
	GL_INT_SAMPLER_2D_ARRAY,
	GL_INT_SAMPLER_2D_MULTISAMPLE,
	GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
	GL_INT_SAMPLER_BUFFER,
	GL_INT_SAMPLER_2D_RECT,
	GL_UNSIGNED_INT_SAMPLER_1D,
	GL_UNSIGNED_INT_SAMPLER_2D,
	GL_UNSIGNED_INT_SAMPLER_3D,
	GL_UNSIGNED_INT_SAMPLER_CUBE,
	GL_UNSIGNED_INT_SAMPLER_1D_ARRAY,
	GL_UNSIGNED_INT_SAMPLER_2D_ARRAY,
	GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,
	GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
	GL_UNSIGNED_INT_SAMPLER_BUFFER,
	GL_UNSIGNED_INT_SAMPLER_2D_RECT};
	for(uint i = 0; i != sizeof(samplers) / sizeof(samplers[0]); i++) {
		if(type == samplers[i]) {
			return true;
		}
	}
	return false;
}

TextureFormat getTextureFormat(ImageFormat format) {
	switch(format) {
		case ImageFormat::Depth32:
			return TextureFormat(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32, GL_FLOAT);
		break;
		case ImageFormat::F32:
			return TextureFormat(GL_RED, GL_R32F, GL_FLOAT);
		break;
		case ImageFormat::RGBA32F:
			return TextureFormat(GL_RGBA, GL_RGBA32F, GL_FLOAT);
		break;
		case ImageFormat::RG16:
			return TextureFormat(GL_RG, GL_RG16, GL_UNSIGNED_SHORT);
		break;
		case ImageFormat::RG16F:
			return TextureFormat(GL_RG, GL_RG16F, GL_FLOAT);
		break;
		case ImageFormat::RG32F:
			return TextureFormat(GL_RG, GL_RG32F, GL_FLOAT);
		break;
		case ImageFormat::RGBA8:
			return TextureFormat(GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
		break;
		case ImageFormat::RGB8:
			return TextureFormat(GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE);
		break;
		case ImageFormat::RGBA16:
			return TextureFormat(GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
		break;
		case ImageFormat::RGBA16F:
			return TextureFormat(GL_RGBA, GL_RGBA16F, GL_FLOAT);
		break;
		default:
			return fatal("Unsuported texture format.");
		break;
	}
}

bool isHWSupported(ImageFormat format) {
	if(format == ImageFormat::RGB10A2) {
		return false;
	}
	static core::Map<ImageFormat, bool> support;
	core::Map<ImageFormat, bool>::const_iterator it = support.find(format);
	if(it == support.end()) {
		GLint s = 0;
		glGetInternalformativ(GL_TEXTURE_2D, getTextureFormat(format).internalFormat, GL_INTERNALFORMAT_SUPPORTED, sizeof(GLint), &s);
		return support[format] = (s == GL_TRUE);
	}
	return (*it)._2;
}

bool isBindlessHandle(UniformType t) {
	return t == GL_UNSIGNED_INT_VEC2;
}



GLenum textureType[] = {GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP};
GLenum bufferType[] = {GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER, GL_UNIFORM_BUFFER, GL_SHADER_STORAGE_BUFFER, GL_DRAW_INDIRECT_BUFFER};
GLenum bufferUsage[] = {GL_STREAM_DRAW, GL_STATIC_DRAW, GL_DYNAMIC_DRAW};
GLenum framebufferType[] = {GL_FRAMEBUFFER, GL_READ_FRAMEBUFFER};
GLenum dataType[] = {GL_NONE, GL_FLOAT, GL_INT, GL_UNSIGNED_INT, GL_SHORT, GL_UNSIGNED_SHORT, GL_BYTE, GL_UNSIGNED_BYTE, GL_DOUBLE};
GLenum textureFilter[] = {GL_NEAREST, GL_LINEAR};
GLenum shaderType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER};
GLenum shaderParam[] = {GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_BLOCKS};
GLenum features[] = {GL_DEPTH_TEST, GL_DEPTH_CLAMP, GL_DEBUG_OUTPUT, GL_DEBUG_OUTPUT_SYNCHRONOUS, GL_TEXTURE_CUBE_MAP_SEAMLESS};
GLenum primitiveMode[] = {GL_TRIANGLES};
GLenum depthMode[] = {GL_LEQUAL, GL_GEQUAL, GL_ALWAYS};
GLenum blendModeSrc[] = {GL_ONE, GL_ONE, GL_SRC_ALPHA};
GLenum blendModeDst[] = {GL_ZERO, GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
GLenum cullMode[] = {GL_BACK, GL_FRONT, GL_NONE};
GLenum intParams[] = {GL_MAX_DRAW_BUFFERS, GL_MAX_TEXTURE_IMAGE_UNITS, GL_MAX_VERTEX_ATTRIBS, GL_MAX_VARYING_VECTORS, GL_MAX_UNIFORM_BLOCK_SIZE, GL_MAX_SHADER_STORAGE_BLOCK_SIZE, GL_MAJOR_VERSION, GL_MINOR_VERSION};
GLenum magSamplers[] {GL_NEAREST, GL_LINEAR};
GLenum minSamplers[] {GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR};
GLenum cubeFace[] = {GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X};


static constexpr uint MaxBindings = 256;
static uint activeTextureUnit = 0;
static Handle boundVao = 0;
Handle boundTextures[MaxBindings] = {0};


GLenum toGLAttachment(uint att) {
	if(att == DepthAtt) {
		return GL_DEPTH_ATTACHMENT;
	}
	if(att == NoAtt) {
		return GL_NONE;
	}
	return GL_COLOR_ATTACHMENT0 + att;
}


GLAPIENTRY void debugOut(GLenum, GLenum type, GLuint, GLuint sev, GLsizei len, const char *msg, const void *) {
	if(sev == GL_DEBUG_SEVERITY_NOTIFICATION) {
		return;
	}
	LogType logT = InfoLog;
	if(type == GL_DEBUG_TYPE_PERFORMANCE) {
		logT = PerfLog;
	}
	if(type == GL_DEBUG_TYPE_ERROR) {
		logT = ErrorLog;
	}
	logMsg((sev == GL_DEBUG_SEVERITY_HIGH ? "[GL][HIGH] " : "[GL] ") + core::String(msg, len), logT);
}







bool initialize() {
	static bool init = false;
	if(!init) {
		glewExperimental = true;
		if(glewInit() != GLEW_OK) {
			logMsg("Unable to initialize glew.", ErrorLog);
			return false;
		}
		logMsg(core::String("Running on ") + glGetString(GL_VENDOR) + " " + glGetString(GL_RENDERER) + " using GL " + glGetString(GL_VERSION) + " using GLSL " + glGetString(GL_SHADING_LANGUAGE_VERSION) + " (" + sizeof(void *) * 8 + " bits)");
		glDebugMessageCallback(&debugOut, 0);
		glGetError();
	}
	return init = true;
}

bool checkError() {
	int error = glGetError();
	if(error) {
		std::cerr<<"Warning : OpenGL error : ";
		switch(error) {
			case GL_INVALID_OPERATION:
				std::cerr<<"INVALID OPERATION";
				break;
			case GL_INVALID_ENUM:
				std::cerr<<"INVALID ENUM";
				break;
			case GL_INVALID_VALUE:
				std::cerr<<"INVALID VALUE";
				break;
			case GL_OUT_OF_MEMORY:
				std::cerr<<"OUT OF MEMORY";
				break;
			default:
				std::cerr<<"error #"<<error;
		}
		std::cerr<<std::endl;
		return true;
	}
	return false;
}



Handle createTexture2D(const math::Vec2ui &size, uint mips, TextureFormat format, const void *data) {
	Handle h = 0;
	glGenTextures(1, &h);
	bindTexture(Texture2D, h);
	glTexStorage2D(GL_TEXTURE_2D,  mips, format.internalFormat, size.x(), size.y());
	if(data) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x(), size.y(), format.format, format.type, data); // crashes if data = 0...
	}
	if(mips > 1) {
		generateMipmap(Texture2D);
	}
	return h;
}


Handle createTextureCube(const math::Vec2ui &size, uint mips, TextureFormat format, const void **data) {
	Handle h = 0;
	glGenTextures(1, &h);
	bindTexture(TextureCube, h);
	glTexStorage2D(GL_TEXTURE_CUBE_MAP,  mips, format.internalFormat, size.x(), size.y());
	for(uint i = 0; i != 6; i++) {
		if(data[i]) {
			glTexSubImage2D(cubeFace[i], 0, 0, 0, size.x(), size.y(), format.format, format.type, data[i]);
		}
	}
	if(mips > 1) {
		generateMipmap(TextureCube);
	}
	return h;
}

Handle createBuffer(BufferTarget target, uint size, const void *data, BufferAlloc usage) {
	Handle h = 0;
	glGenBuffers(1, &h);
	bindBuffer(target, h);
	//glBufferStorage(bufferType[target], size, data, GL_MAP_WRITE_BIT | (usage == Static ? 0 : GL_DYNAMIC_STORAGE_BIT));
	glBufferData(bufferType[target], size, data, bufferUsage[usage]);
	return h;
}

Handle createVertexArray() {
	Handle h = 0;
	glGenVertexArrays(1, &h);
	return h;
}

Handle createFramebuffer() {
	Handle h = 0;
	glGenFramebuffers(1, &h);
	return h;
}

void deleteTexture(Handle handle) {
	glDeleteTextures(1, &handle);
}

void deleteBuffer(Handle handle) {
	glDeleteBuffers(1, &handle);
}

void deleteVertexArray(Handle handle) {
	glDeleteVertexArrays(1, &handle);
}

void deleteFramebuffer(Handle handle) {
	glDeleteFramebuffers(1, &handle);
}

void updateBuffer(BufferTarget target, uint size, const void *data, BufferAlloc usage) {
	glBufferData(bufferType[target], size, 0, bufferUsage[usage]);
	glBufferSubData(bufferType[target], 0, size, data);
}

void bindTexture(TextureType type, Handle tex) {
	if(boundTextures[activeTextureUnit] != tex) {
		glBindTexture(textureType[type], tex);
		boundTextures[activeTextureUnit] = tex;
	}
}

void setActiveTexture(uint slot) {
	if(activeTextureUnit != slot) {
		glActiveTexture(GL_TEXTURE0 + slot);
		activeTextureUnit = slot;
	}
}

void bindTextureUnit(uint slot, TextureType type, Handle tex) {
	if(boundTextures[slot] != tex) {
		setActiveTexture(slot);
		bindTexture(type, tex);
	}
}

void bindSampler(uint slot, Handle sampler) {
	static Handle bound[MaxBindings] = {0};
	if(bound[slot] != sampler) {
		glBindSampler(slot, sampler);
		bound[slot] = sampler;
	}
}

Handle createShader(ShaderType type) {
	return glCreateShader(shaderType[type]);
}

Handle createSampler(TextureSampler sampler, bool mipmap) {
	Handle h = 0;
	glGenSamplers(1, &h);
	glSamplerParameteri(h, GL_TEXTURE_MAG_FILTER, magSamplers[sampler & ~(Clamp | MipmapLinear)]);
	if(mipmap) {
		glSamplerParameteri(h, GL_TEXTURE_MIN_FILTER, minSamplers[sampler & ~Clamp]);
	} else {
		glSamplerParameteri(h, GL_TEXTURE_MIN_FILTER, magSamplers[sampler & ~(Clamp | MipmapLinear)]);
	}

	if(sampler & Clamp) {
		glSamplerParameteri(h, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(h, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glSamplerParameteri(h, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	} else {
		glSamplerParameteri(h, GL_TEXTURE_WRAP_R, GL_REPEAT);
		glSamplerParameteri(h, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glSamplerParameteri(h, GL_TEXTURE_WRAP_S, GL_REPEAT);
	}
	return h;
}

void shaderSource(Handle shader, uint count, const char * const *src, const int *len) {
	glShaderSource(shader, count, src, len);
}

bool compileShader(Handle shader) {
	core::Timer timer;
	glCompileShader(shader);
	int res = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
	if(timer.elapsed() > 0.1) {
		logMsg("[GL] Shader compilation took more than 100 ms !", WarningLog);
	}
	return res;
}

void getShaderiv(Handle shader, ShaderParam param, int *i) {
	glGetShaderiv(shader, shaderParam[param], i);
}

void getShaderInfoLog(Handle shader, uint max, int *len, char *log) {
	glGetShaderInfoLog(shader, max, len, log);
}

void deleteShader(Handle shader) {
	glDeleteShader(shader);
}

Handle createProgram() {
	return glCreateProgram();
}

void deleteProgram(Handle prog) {
	glDeleteProgram(prog);
}

void useProgram(Handle prog) {
	glUseProgram(prog);
}

void attachShader(Handle prog, Handle shader) {
	glAttachShader(prog, shader);
}

bool linkProgram(Handle prog) {
	core::Timer timer;
	glLinkProgram(prog);
	int res = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &res);
	if(timer.elapsed() > 0.25) {
		logMsg("[GL] Shader linking took more than 250 ms !", WarningLog);
	}
	return res;
}

core::String getProgramInfoLog(Handle prog) {
	int len = 0;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
	char *log = new char[len + 1];
	glGetProgramInfoLog(prog, len + 1, &len, log);
	core::String str(log, len);
	delete[] log;
	return str;
}

core::String getShaderInfoLog(Handle shader) {
	int len = 0;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	char *log = new char[len + 1];
	glGetShaderInfoLog(shader, len + 1, &len, log);
	core::String str(log, len);
	delete[] log;
	return str;
}

int getProgramInt(Handle prog, ShaderParam param) {
	int res = 0;
	if(param == ActiveBuffers) {
		glGetProgramInterfaceiv(prog, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &res);
	} else {
		glGetProgramiv(prog, shaderParam[param], &res);
	}
	return res;
}

core::String getActiveUniformInfo(Handle prog, uint index, uint *size, UniformType *type) {
	static constexpr uint MaxSize = 1024;
	static char nameBuffer[1024];
	int len = 0;
	UniformType ty = 0;
	int si = 0;
	glGetActiveUniform(prog, index, MaxSize, &len, &si, &ty, nameBuffer);
	if(size) {
		*size = si;
	}
	if(type) {
		*type = ty;
	}
	return core::String(nameBuffer, len);
}

core::String getActiveUniformBlockName(Handle prog, uint index) {
	static constexpr uint MaxSize = 1024;
	static char nameBuffer[1024];
	int len = 0;
	glGetActiveUniformBlockName(prog, index, MaxSize, &len, nameBuffer);
	return core::String(nameBuffer, len);
}


core::String getActiveBufferName(Handle prog, uint index) {
	static constexpr uint MaxSize = 1024;
	static char nameBuffer[1024];
	int len = 0;
	glGetProgramResourceName(prog, GL_SHADER_STORAGE_BLOCK, index, MaxSize, &len, nameBuffer);
	return core::String(nameBuffer, len);
}

UniformAddr getUniformLocation(Handle shader, const core::String &name) {
	return getUniformLocation(shader, name.toChar());
}

UniformAddr getUniformLocation(Handle shader, const char *name) {
	return glGetUniformLocation(shader, name);
}

void setEnabled(Feature feat, bool e) {
	static bool feats[MaxFeatures] = {false};
	if(feats[feat] == e) {
		return;
	}
	if((feats[feat] = e)) {
		glEnable(features[feat]);
	} else {
		glDisable(features[feat]);
	}
}

void setDepthMode(DepthMode mode) {
	static DepthMode current = DepthMode::Lesser;
	if(mode != current) {
		glDepthFunc(depthMode[current = mode]);
	}
}

void setBlendMode(BlendMode mode) {
	static BlendMode current = BlendMode::DontBlend;
	if(mode != current) {
		if(mode == BlendMode::DontBlend) {
			glDisable(GL_BLEND);
		} else {
			if(current == BlendMode::DontBlend) {
				glEnablei(GL_BLEND, 0);
			}
			glBlendFunc(blendModeSrc[mode], blendModeDst[mode]);
		}
		current = mode;
	}
}

void setDepthMask(bool mask) {
	static bool m = true;
	if(mask != m) {
		glDepthMask(m = mask);
	}
}

void setCullFace(CullMode mode) {
	static CullMode current = CullMode::DontCull;
	if(mode != current) {
		if(mode == CullMode::DontCull) {
			glDisable(GL_CULL_FACE);
		} else {
			if(current == CullMode::DontCull) {
				glEnable(GL_CULL_FACE);
			}
			glCullFace(cullMode[mode]);
		}
		current = mode;
	}
}

void setColorMask(bool r, bool g, bool b, bool a) {
	glColorMask(r, g, b, a);
}

void setUniformBlockBinding(Handle prog, const core::String &name, uint binding) {
	setUniformBlockBinding(prog, name.toChar(), binding);
}

void setUniformBlockBinding(Handle prog, const char *name, uint binding) {
	glUniformBlockBinding(prog, glGetUniformBlockIndex(prog, name), binding);
}


void setStorageBlockBinding(Handle prog, const core::String &name, uint binding) {
	setStorageBlockBinding(prog, name.toChar(), binding);
}

void setStorageBlockBinding(Handle prog, const char *name, uint binding) {
	glShaderStorageBlockBinding(prog, glGetProgramResourceIndex(prog, GL_SHADER_STORAGE_BLOCK, name), binding);
}

void setViewport(math::Vec2i a, math::Vec2i b) {
	glViewport(a.x(), a.y(), b.x(), b.y());
}

int getInt(IntParam i) {
	int v = 0;
	glGetIntegerv(intParams[i], &v);
	return v;
}

bool isExtensionSupported(core::String extName) {
	static core::String exts;
	if(exts.isNull()) {
		int extCount = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);
		for(int i = 0; i != extCount; i++) {
			exts += reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, i));
		}
	}
	return exts.contains(extName);
}










void bindBuffer(BufferTarget target, Handle buffer) {
	glBindBuffer(bufferType[target], buffer);
}

void bindBufferBase(BufferTarget target, uint index, Handle buffer) {
	glBindBufferBase(bufferType[target], index, buffer);
}

void bindVertexArray(Handle array) {
	if(boundVao != array) {
		glBindVertexArray(boundVao = array);
	}
}

void vertexAttribPointer(uint index, uint size, Type type, bool norm, uint stride, const void *ptr, uint divisor) {
	if(type == Float || type == Double) {
		glVertexAttribPointer(index, size, dataType[type], norm, stride, ptr);
	} else {
		glVertexAttribIPointer(index, size, dataType[type], stride, ptr);
	}
	if(divisor) {
		glVertexAttribDivisor(index, divisor);
	}
}

void enableVertexAttribArray(uint index) {
	glEnableVertexAttribArray(index);
}

void framebufferTexture2D(FrameBufferType target, Attachment attachement, TextureType texture, Handle handle, uint level) {
	glFramebufferTexture2D(framebufferType[target], toGLAttachment(attachement), textureType[texture], handle, level);
}

void attachFramebufferTexture(uint att, TextureType tex, Handle handle) {
	glFramebufferTexture2D(framebufferType[FrameBuffer], toGLAttachment(att), textureType[tex], handle, 0);
}

void attachFramebufferTextureCube(Handle handle) {
	for(uint i = 0; i != 6; i++) {
		glFramebufferTexture2D(framebufferType[FrameBuffer], GL_COLOR_ATTACHMENT0 + i, cubeFace[i], handle, 0);
	}
}

void assertFboStatus() {
	if(gl::checkFramebufferStatus(gl::FrameBuffer) != gl::FboOk) {
		switch(gl::checkFramebufferStatus(gl::FrameBuffer)) {
			case gl::FboMissingAtt:
				fatal("Unable to modify frame-buffer-object : missing attachment.");
			break;
			case gl::FboIncomplete:
				fatal("Unable to modify frame-buffer-object : incomplete attachment.");
			break;
			case gl::FboUnsupported:
				fatal("Unable to modify frame-buffer-object : unsuported feature.");
			break;
			default:
				fatal("Unable to modify frame-buffer-object.");
			break;
		}
	}
}

void drawNoAttrib(uint count) {
	static Handle vao = 0;
	if(!vao) {
		vao = createVertexArray();
	}
	bindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, count);
}

void drawBuffers(uint count, const Attachment *buffers) {
	GLenum *att = new GLenum[count];
	for(uint i = 0; i != count; i++) {
		att[i] = toGLAttachment(buffers[i]);
	}
	glDrawBuffers(count, att);
	delete[] att;
}

FrameBufferStatus checkFramebufferStatus(FrameBufferType framebuffer) {
	GLenum ok = glCheckFramebufferStatus(framebufferType[framebuffer]);
	if(ok != GL_FRAMEBUFFER_COMPLETE) {
		switch(ok) {
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				return FboMissingAtt;
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				return FboIncomplete;
			break;
			default:
				return FboUnsupported;
			break;
		}
	}
	return FboOk;
}


void bindFramebuffer(FrameBufferType target, Handle fbo) {
	glBindFramebuffer(framebufferType[target], fbo);
}

void readBuffer(Attachment attachment) {
	glReadBuffer(toGLAttachment(attachment));
}

void clear(BitField buffers) {
	GLbitfield b = (buffers & ColorBit ? GL_COLOR_BUFFER_BIT : 0) | (buffers & DepthBit ? GL_DEPTH_BUFFER_BIT : 0);
	glClear(b);
}

void blitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, BitField mask, Filter filter) {
	GLbitfield b = (mask & ColorBit ? GL_COLOR_BUFFER_BIT : 0) | (mask & DepthBit ? GL_DEPTH_BUFFER_BIT : 0);
	glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, b, textureFilter[filter]);
}

void drawElementsInstancedBaseVertex(PrimitiveType mode, uint count, void *indices, uint primCount, uint baseVertex) {
	glDrawElementsInstancedBaseVertex(primitiveMode[mode], count, dataType[GLType<uint>::value], indices, primCount, baseVertex);
}

void multiDrawElementsIndirect(PrimitiveType mode, uint cmdCount) {
	glMultiDrawElementsIndirect(primitiveMode[mode], dataType[GLType<uint>::value], 0, cmdCount, 0);
}

void generateMipmap(TextureType type) {
	glGenerateMipmap(textureType[type]);
}

uint64 getTextureBindlessHandle(Handle tex, TextureSampler smp, bool mipmap) {
	static Handle samplers[TextureSampler::MaxSamplers][2] = {{0}, {0}};
	if(!samplers[smp][mipmap]) {
		samplers[smp][mipmap] = createSampler(smp, mipmap);
	}
	return glGetTextureSamplerHandleARB(tex, samplers[smp][mipmap]);
}

void makeTextureHandleResident(uint64 handle) {
	glMakeTextureHandleResidentARB(handle);
}

void programUniform1iv(Handle prog, UniformAddr loc, uint count, const int *a) {
	glProgramUniform1iv(prog, loc, count, a);
}

void programUniform1uiv(Handle prog, UniformAddr loc, uint count, const uint *a) {
	glProgramUniform1uiv(prog, loc, count, a);
}

void programUniform1fv(Handle prog, UniformAddr loc, uint count, const float *a) {
	glProgramUniform1fv(prog, loc, count, a);
}

void programUniform2iv(Handle prog, UniformAddr loc, uint count, const int *v) {
	glProgramUniform2iv(prog, loc, count, v);
}

void programUniform3iv(Handle prog, UniformAddr loc, uint count, const int *v) {
	glProgramUniform3iv(prog, loc, count, v);
}

void programUniform2fv(Handle prog, UniformAddr loc, uint count, const float *v) {
	glProgramUniform2fv(prog, loc, count, v);
}

void programUniform3fv(Handle prog, UniformAddr loc, uint count, const float *v) {
	glProgramUniform3fv(prog, loc, count, v);
}

void programUniform4fv(Handle prog, UniformAddr loc, uint count, const float *v) {
	glProgramUniform4fv(prog, loc, count, v);
}

void programUniformMatrix2fv(Handle prog, UniformAddr loc, uint count, bool tr, const float *m) {
	glProgramUniformMatrix2fv(prog, loc, count, tr, m);
}

void programUniformMatrix3fv(Handle prog, UniformAddr loc, uint count, bool tr, const float *m) {
	glProgramUniformMatrix3fv(prog, loc, count, tr, m);
}

void programUniformMatrix4fv(Handle prog, UniformAddr loc, uint count, bool tr, const float *m) {
	glProgramUniformMatrix4fv(prog, loc, count, tr, m);
}

void programUniformHandleui64(Handle prog, UniformAddr loc, uint64 handle) {
	glProgramUniformHandleui64ARB(prog, loc, handle);
}

void flush() {
	glFlush();
}



}
}
}

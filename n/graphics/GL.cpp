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
#include <GL/glew.h>

#define N_OGL_DOES_NOTHING

namespace n {
namespace graphics {
namespace gl {

bool isSampler(UniformType type) {
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
			return TextureFormat(GL_RG, GL_RG16F, GL_UNSIGNED_SHORT);
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
			return TextureFormat(GL_RGBA, GL_RGBA16F, GL_UNSIGNED_SHORT);
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

#ifdef N_OGL_DOES_NOTHING

void genDummies(uint c, Handle *x) {
	for(uint i = 0; i != c; i++) {
		x[i] = 1;
	}
}

void genTextures(uint c, Handle *t) { genDummies(c, t); }
void genBuffers(uint c, Handle *t) { genDummies(c, t); }
void genVertexArrays(uint c, Handle *t) { genDummies(c, t); }
void genFramebuffers(uint c, Handle *t) { genDummies(c, t); }
void deleteTextures(uint, const Handle *) {}
void deleteBuffers(uint, const Handle *) {}
void deleteVertexArrays(uint, const Handle *) {}
void deleteFramebuffers(uint, const Handle *) {}
void bindTexture(TextureType, Handle) {}
void activeTexture(uint) {}
void bindTextureUnit(uint, Handle) {}
void bindSampler(uint, Handle) {}
void bindBuffer(BufferBinding, Handle) {}
void bindBufferBase(BufferBinding, uint, Handle) {}
void bufferData(BufferBinding, uint, const void *, BufferAlloc) {}
void bufferSubData(BufferBinding, uint, uint, const void *) {}
void bindVertexArray(Handle) {}
void vertexAttribPointer(uint, uint, Type, bool, uint, const void *) {}
void enableVertexAttribArray(uint) {}
void framebufferTexture2D(FrameBufferType, Attachment, TextureType, Handle, uint) {}
void drawBuffers(uint, const Attachment *) {}
FrameBufferStatus checkFramebufferStatus(FrameBufferType) { return FboOk; }
void bindFramebuffer(FrameBufferType, Handle) {}
void readBuffer(Attachment) {}
void clear(BitField) {}
void blitFramebuffer(int, int, int, int, int, int, int, int, BitField, Filter) {}
Handle createShader(ShaderType) { return 1; }
void shaderSource(Handle, uint, const char **, const uint *) {}
void compileShader(Handle) {}
void getShaderiv(Handle, ShaderParam, int *i) { *i = 1; }
void getShaderInfoLog(Handle, uint, int *, char *) {}
void deleteShader(Handle) {}
Handle createProgram() { return 1; }
void deleteProgram(Handle) {}
void useProgram(Handle) {}
void attachShader(Handle, Handle) {}
void linkProgram(Handle) {}
void getProgramiv(Handle, ShaderParam p, int *i) { *i = p == gl::LinkStatus; }
void getProgramInfoLog(Handle, uint, int *, char *) {}
void getActiveUniform(Handle, uint, uint, uint *, int *, UniformType *, char *) {}
void getActiveUniformBlockName(Handle, uint, uint, int *, char *) {}
int getUniformLocation(Handle, const char *) { return InvalidIndex; }
uint getUniformBlockIndex(Handle, const char *) { return 0; }
void uniformBlockBinding(Handle, uint, uint) {}
void viewport(int, int, int, int) {}
void drawElementsInstancedBaseVertex(PrimitiveType, uint, Type, void *, uint, uint) {}
void enable(Feature) {}
void disable(Feature) {}
void depthFunc(DepthMode) {}
void blendFunc(BlendMode) {}
void depthMask(bool) {}
void cullFace(CullMode) {}
void colorMask(bool, bool, bool, bool) {}
void programUniform1iv(Handle, int, uint, const int *) {}
void programUniform1uiv(Handle, int, uint, const uint *) {}
void programUniform1fv(Handle, int, uint, const float *) {}
void programUniform2iv(Handle, int, uint, const int *) {}
void programUniform3iv(Handle, int, uint, const int *) {}
void programUniform2fv(Handle, int, uint, const float *) {}
void programUniform3fv(Handle, int, uint, const float *) {}
void programUniform4fv(Handle, int, uint, const float *) {}
void programUniformMatrix2fv(Handle, int, uint, bool, const float *) {}
void programUniformMatrix3fv(Handle, int, uint, bool, const float *) {}
void programUniformMatrix4fv(Handle, int, uint, bool, const float *) {}
void programUniformHandleui64(Handle, int, uint64) {}
void texImage2D(TextureType, int, uint, uint, int, TextureFormat, const void *) {}
void generateMipmap(TextureType) {}
uint64 getTextureSamplerHandle(Handle) { return 1; }
void makeTextureHandleResident(uint64) {}

#endif




}
}
}

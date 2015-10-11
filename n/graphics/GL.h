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
along with this program. If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_GRAPHICS_GL
#define N_GRAPHICS_GL

#include <n/defines.h>
#include <n/types.h>
#include <n/math/Vec.h>
#include "Color.h"

namespace n {
namespace graphics {

enum BufferBinding
{
	ArrayBuffer,
	IndexBuffer,
	UniformArrayBuffer
};

enum TextureType
{
	Texture2D,
	TextureCube
};

enum CullMode : uint16
{
	DontCull = 2,
	Back = 0,
	Front = 1
};

enum BlendMode : uint16
{
	DontBlend = 0,
	Add = 1,
	SrcAlpha = 2
};

enum DepthMode : uint16
{
	Lesser = 0,
	Greater = 1,
	Always = 2
};

enum ShaderType
{
	FragmentShader,
	VertexShader,
	GeometryShader
};

namespace gl {

struct TextureFormat;

typedef uint GLuint;
typedef uint GLsizei;
typedef uint GLenum;
typedef int GLint;
typedef uint Handle;
typedef uint BitField;
typedef uint UniformType;

static constexpr BitField ColorBit = 0x01;
static constexpr BitField DepthBit = 0x02;
static constexpr int InvalidIndex = -1;
static constexpr uint InvalidType = uint(-1);

TextureFormat getTextureFormat(ImageFormat format);
bool isBindlessHandle(UniformType t);
bool isSampler(UniformType type);

enum Feature
{
	DepthTest,
	DepthClamp,
	CullFace,
	Blend
};

enum BufferAlloc
{
	Stream,
	Static,
	Dynamic
};

enum Type
{
	None,
	Float,
	Int,
	UInt,
	Short,
	UShort,
	Char,
	Byte,
	Double,
};

enum PrimitiveType
{
	Triangles
};

enum Attachment : uint
{
	NoAtt = uint(-1),
	DepthAtt = 0,
	ColorAtt0 = 1
};

enum FrameBufferStatus
{
	FboOk,
	FboIncomplete,
	FboMissingAtt,
	FboUnsupported
};

enum FrameBufferType
{
	FrameBuffer,
	ReadBuffer
};

enum Filter
{
	Nearest
};

enum ShaderParam
{
	CompileResult,
	LogLength,
	LinkStatus,
	ActiveUniforms,
	ActiveBlocks
};

struct TextureFormat
{
	private:
		friend TextureFormat getTextureFormat(ImageFormat);
		friend bool isHWSupported(ImageFormat);

		TextureFormat(uint f, uint i, uint t) : format(f), internalFormat(i), type(t) {
		}

		const uint format;
		const uint internalFormat;
		const uint type;
};

bool isHWSupported(ImageFormat format);
void genTextures(uint count, Handle *tex);
void deleteTextures(uint count, const Handle *tex);
void genBuffers(uint c, Handle *buffers);
void deleteBuffers(uint count, const Handle *buffers);
void genVertexArrays(uint count, Handle *arrays);
void deleteVertexArrays(uint count, const Handle *arrays);
void genFramebuffers(uint count, Handle *fbo);
void deleteFramebuffers(uint count, const Handle *fbo);
void bindTexture(TextureType t, Handle tex);
void activeTexture(uint s);
void bindTextureUnit(uint slot, Handle h);
void bindSampler(uint slot, Handle s);
void bindBuffer(BufferBinding b, Handle buffer);
void bindBufferBase(BufferBinding b, uint index, Handle buffer);
void bufferData(BufferBinding t, uint size, const void *data, BufferAlloc alloc);
void bufferSubData(BufferBinding t, uint start, uint size, const void *data);
void bindVertexArray(Handle vao);
void vertexAttribPointer(uint index, uint size, Type t, bool transpose, uint stride, const void *buffer);
void enableVertexAttribArray(uint index);
void framebufferTexture2D(FrameBufferType t, Attachment attachement, TextureType tt, Handle handle, uint mip);
void drawBuffers(uint count, const Attachment *att);
FrameBufferStatus checkFramebufferStatus(FrameBufferType t);
void bindFramebuffer(FrameBufferType, Handle fbo);
void readBuffer(Attachment att);
void clear(BitField targets);
void blitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, BitField mask, Filter filter);
Handle createShader(ShaderType type);
void shaderSource(Handle h, uint srcCount, const char **srcs, const uint *len);
void compileShader(Handle h);
void getShaderiv(Handle h, ShaderParam p, int *i);
void getShaderInfoLog(Handle h, uint maxSize, int *len, char *log);
void deleteShader(Handle h);
Handle createProgram();
void deleteProgram(Handle p);
void useProgram(Handle p);
void attachShader(Handle p, Handle s);
void linkProgram(Handle h);
void getProgramiv(Handle h, ShaderParam p, int *i);
void getProgramInfoLog(Handle h, uint maxSize, int *len, char *log);
void getActiveUniform(Handle p, uint index, uint max, uint *len, int *size, UniformType *param, char *n);
void getActiveUniformBlockName(Handle p, uint index, uint max, int *len, char *name);
int getUniformLocation(Handle h, const char *n);
uint getUniformBlockIndex(Handle h, const char *n);
void uniformBlockBinding(Handle p, uint index, uint binding);
void viewport(int srcX0, int srcY0, int srcX1, int srcY1);
void drawElementsInstancedBaseVertex(PrimitiveType p, uint pCount, Type t, void *start, uint instances, uint vertexBase);
void enable(Feature f);
void disable(Feature f);
void depthFunc(DepthMode m);
void blendFunc(BlendMode m);
void depthMask(bool m);
void cullFace(CullMode m);
void colorMask(bool r, bool g, bool b, bool a);
void programUniform1iv(Handle h, int loc, uint count, const int *a);
void programUniform1uiv(Handle h, int loc, uint count, const uint *a);
void programUniform1fv(Handle h, int loc, uint count, const float *f);
void programUniform2iv(Handle h, int loc, uint count, const int *v);
void programUniform3iv(Handle h, int loc, uint count, const int *v);
void programUniform2fv(Handle h, int loc, uint count, const float *v);
void programUniform3fv(Handle h, int loc, uint count, const float *v);
void programUniform4fv(Handle h, int loc, uint count, const float *v);
void programUniformMatrix2fv(Handle h, int loc, uint count, bool tr, const float *m);
void programUniformMatrix3fv(Handle h, int loc, uint count, bool tr, const float *m);
void programUniformMatrix4fv(Handle h, int loc, uint count, bool tr, const float *m);
void programUniformHandleui64(Handle h, int loc, uint64 handle);
void texImage2D(TextureType t, int level, uint w, uint h, int b, TextureFormat f, const void *d);
void generateMipmap(TextureType t);
uint64 getTextureSamplerHandle(Handle h);
void makeTextureHandleResident(uint64 h);
}


template<typename T>
struct GLType
{
	static constexpr gl::Type value = gl::None;
	static constexpr uint size = 0;
};

template<>
struct GLType<float>
{
	static_assert(sizeof(float) == 4, "float should be 4 byte long.");
	static constexpr gl::Type value = gl::Float;
	static constexpr uint size = 1;
};

template<>
struct GLType<double>
{
	static_assert(sizeof(double) == 8, "double should be 8 byte long.");
	static constexpr gl::Type value = gl::Double;
	static constexpr uint size = 1;
};

template<>
struct GLType<int8>
{
	static constexpr gl::Type value = gl::Char;
	static constexpr uint size = 1;
};

template<>
struct GLType<uint8>
{
	static constexpr gl::Type value = gl::Byte;
	static constexpr uint size = 1;
};

template<>
struct GLType<int16>
{
	static constexpr gl::Type value = gl::Short;
	static constexpr uint size = 1;
};

template<>
struct GLType<uint16>
{
	static constexpr gl::Type value = gl::UShort;
	static constexpr uint size = 1;
};

template<>
struct GLType<int32>
{
	static constexpr gl::Type value = gl::Int;
	static constexpr uint size = 1;
};

template<>
struct GLType<uint32>
{
	static constexpr gl::Type value = gl::UInt;
	static constexpr uint size = 1;
};

template<uint N, typename T>
struct GLType<math::Vec<N, T>>
{
	static constexpr gl::Type value = GLType<T>::value;
	static constexpr uint size = N;
};


bool isSampler(gl::GLenum type);

}
}
//}

#endif // N_GRAPHICS_GL


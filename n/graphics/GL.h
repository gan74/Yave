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

enum BufferTarget
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

typedef uint Handle;
typedef uint BitField;
typedef uint UniformType;
typedef int UniformAddr;

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
	DepthClamp
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
	DepthAtt = uint(-2),
	ColorAtt0 = 0
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
		friend void texImage2D(TextureType, int, uint, uint, int, TextureFormat, const void *);

		TextureFormat(uint f, uint i, uint t) : format(f), internalFormat(i), type(t) {
		}

		const uint format;
		const uint internalFormat;
		const uint type;
};


bool isSampler(UniformType type);
bool isHWSupported(ImageFormat format);
Handle createProgram();
Handle createShader(ShaderType shaderType);
void useProgram(Handle prog);

void setDepthMode(DepthMode mode);
void setCullFace(CullMode mode);
void setBlendMode(BlendMode mode);
void setEnabled(Feature feat, bool e);
void setDepthMask(bool mask);
void setColorMask(bool r, bool g, bool b, bool a);

void genTextures(uint count, Handle *textures);
void deleteTextures(uint count, const Handle *textures);
void genBuffers(uint c, Handle *buffers);
void deleteBuffers(uint count, const Handle *buffers);
void genVertexArrays(uint count, Handle *arrays);
void deleteVertexArrays(uint count, const Handle *arrays);
void genFramebuffers(uint count, Handle *framebuffers);
void deleteFramebuffers(uint count, const Handle *fbo);
void bindTexture(TextureType type, Handle tex);
void activeTexture(uint s);
void bindTextureUnit(uint slot, Handle tex);
void bindSampler(uint slot, Handle sampler);
void bindBuffer(BufferTarget binding, Handle buffer);
void bindBufferBase(BufferTarget target, uint index, Handle buffer);
void bufferData(BufferTarget target, uint size, const void *data, BufferAlloc usage);
void bufferSubData(BufferTarget t, uint offset, uint start, const void *data);
void bindVertexArray(Handle array);
void vertexAttribPointer(uint index, uint size, Type type, bool norm, uint stride, const void *ptr);
void enableVertexAttribArray(uint index);
void framebufferTexture2D(FrameBufferType target, Attachment attachement, TextureType texture, Handle handle, uint level);
void drawBuffers(uint count, const Attachment *att);
FrameBufferStatus checkFramebufferStatus(FrameBufferType framebuffer);
void bindFramebuffer(FrameBufferType target, Handle fbo);
void readBuffer(Attachment att);
void clear(BitField buffers);
void blitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, BitField mask, Filter filter);
void shaderSource(Handle shader, uint count, const char * const *src, const int *len);
void compileShader(Handle shader);
void getShaderiv(Handle shader, ShaderParam param, int *i);
void getShaderInfoLog(Handle shader, uint max, int *len, char *log);
void deleteShader(Handle shader);
void deleteProgram(Handle prog);
void attachShader(Handle porg, Handle shader);
void linkProgram(Handle prog);
void getProgramiv(Handle prog, ShaderParam param, int *i);
void getProgramInfoLog(Handle h, uint maxSize, int *len, char *log);
void getActiveUniform(Handle prog, uint index, uint max, int *len, int *size, UniformType *type, char *name);
void getActiveUniformBlockName(Handle prog, uint index, uint max, int *len, char *name);
UniformAddr getUniformLocation(Handle shader, const char *name);
uint getUniformBlockIndex(Handle shader, const char *name);
void uniformBlockBinding(Handle prog, uint index, uint binding);
void viewport(int srcX0, int srcY0, int srcX1, int srcY1);
void drawElementsInstancedBaseVertex(PrimitiveType mode, uint count, Type p, void *indices, uint primCount, uint baseVertex);
void programUniform1iv(Handle h, UniformAddr loc, uint count, const int *a);
void programUniform1uiv(Handle h, UniformAddr loc, uint count, const uint *a);
void programUniform1fv(Handle h, UniformAddr loc, uint count, const float *a);
void programUniform2iv(Handle h, UniformAddr loc, uint count, const int *v);
void programUniform3iv(Handle h, UniformAddr loc, uint count, const int *v);
void programUniform2fv(Handle h, UniformAddr loc, uint count, const float *v);
void programUniform3fv(Handle h, UniformAddr loc, uint count, const float *v);
void programUniform4fv(Handle h, UniformAddr loc, uint count, const float *v);
void programUniformMatrix2fv(Handle h, UniformAddr loc, uint count, bool tr, const float *m);
void programUniformMatrix3fv(Handle h, UniformAddr loc, uint count, bool tr, const float *m);
void programUniformMatrix4fv(Handle h, UniformAddr loc, uint count, bool tr, const float *m);
void programUniformHandleui64(Handle prog, UniformAddr loc, uint64 handle);
void texImage2D(TextureType target, int level, uint width, uint height, int border, TextureFormat format, const void *data);
void generateMipmap(TextureType type);
uint64 getTextureSamplerHandle(Handle tex);
void makeTextureHandleResident(uint64 handle);
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

}
}
//}

#endif // N_GRAPHICS_GL


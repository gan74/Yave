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

enum TextureSampler
{
	Nearest,
	Bilinear,
	Trilinear,

	Default
};


enum BufferTarget
{
	ArrayBuffer,
	IndexBuffer,
	UniformBufferObject,
	StorageBufferObject,
	DrawIndirectBuffer
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
bool isSamplerType(UniformType type);

enum Feature
{
	DepthTest,
	DepthClamp,
	Debug,
	SynchronousDebugging,
	SeamlessCubeMaps,


	MaxFeatures
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
	ActiveUniforms,
	ActiveBlocks,
	ActiveBuffers
};

enum IntParam
{
	MaxFboAttachements,
	MaxTextureUnits,
	MaxVertexAttrib,
	MaxVaryingVectors,
	MaxUBOSize,
	MaxSSBOSize,
	MajorVersion,
	MinorVersion
};

struct DrawCommand
{
	uint count;
	uint instanceCount;
	void *start;
	uint baseVertex;
	uint baseInstance;
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

bool initialize();
bool checkError();

bool isSamplerType(UniformType type);
bool isHWSupported(ImageFormat format);
void useProgram(Handle prog);
bool linkProgram(Handle prog);
bool compileShader(Handle shader);
core::String getProgramInfoLog(Handle prog);
core::String getShaderInfoLog(Handle shader);
int getProgramInt(Handle prog, ShaderParam param);

void setDepthMode(DepthMode mode);
void setCullFace(CullMode mode);
void setBlendMode(BlendMode mode);
void setEnabled(Feature feat, bool e = true);
void setDepthMask(bool mask);
void setColorMask(bool r, bool g, bool b, bool a);

Handle createProgram();
Handle createShader(ShaderType shaderType);
Handle createSampler(TextureSampler sampler, bool mipmap);
Handle createTexture();
Handle createBuffer(BufferTarget target, uint size, const void *data, BufferAlloc usage);
Handle createVertexArray();
Handle createFramebuffer();

void deleteTexture(Handle handle);
void deleteBuffer(Handle handle);
void deleteVertexArray(Handle handle);
void deleteFramebuffer(Handle handle);
void deleteShader(Handle shader);
void deleteProgram(Handle prog);

void bindTexture(TextureType type, Handle tex);
void setActiveTexture(uint s);
void bindTextureUnit(uint slot, TextureType type, Handle tex);
void bindSampler(uint slot, Handle sampler);

core::String getActiveUniformInfo(Handle prog, uint index, uint *size, UniformType *type);
UniformAddr getUniformLocation(Handle shader, const char *name);
UniformAddr getUniformLocation(Handle shader, const core::String &name);
core::String getActiveUniformBlockName(Handle prog, uint index);
core::String getActiveBufferName(Handle prog, uint index);
uint getUniformBlockIndex(Handle shader, const core::String &name);
uint getUniformBlockIndex(Handle shader, const char *name);

void setUniformBlockBinding(Handle prog, const core::String &name, uint binding);
void setUniformBlockBinding(Handle prog, const char *name, uint binding);

void setStorageBlockBinding(Handle prog, const core::String &name, uint binding);
void setStorageBlockBinding(Handle prog, const char *name, uint binding);

void attachShader(Handle porg, Handle shader);

void multiDrawElementsIndirect(PrimitiveType mode, uint cmdCount);

void setViewport(math::Vec2i a, math::Vec2i b);
int getInt(IntParam i);
bool isExtensionSupported(core::String extName);



//    vvvvvvvvvvvv   Unfinished    vvvvvvvvvvvv


void bindBuffer(BufferTarget binding, Handle buffer);
void bindBufferBase(BufferTarget target, uint index, Handle buffer);
void bufferSubData(BufferTarget t, uint offset, uint start, const void *data);
void bindVertexArray(Handle array);
void vertexAttribPointer(uint index, uint size, Type type, bool norm, uint stride, const void *ptr, uint divisor = 0);
void enableVertexAttribArray(uint index);
void framebufferTexture2D(FrameBufferType target, Attachment attachement, TextureType texture, Handle handle, uint level);
void drawBuffers(uint count, const Attachment *att);
FrameBufferStatus checkFramebufferStatus(FrameBufferType framebuffer);
void bindFramebuffer(FrameBufferType target, Handle fbo);
void readBuffer(Attachment att);
void clear(BitField buffers);
void blitFramebuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, BitField mask, Filter filter);
void shaderSource(Handle shader, uint count, const char * const *src, const int *len);
void getShaderiv(Handle shader, ShaderParam param, int *i);
void drawElementsInstancedBaseVertex(PrimitiveType mode, uint count, void *indices, uint primCount, uint baseVertex);
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
uint64 getTextureSamplerHandle(Handle tex, TextureSampler smp, bool mipmap);
void makeTextureHandleResident(uint64 handle);
void flush();
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


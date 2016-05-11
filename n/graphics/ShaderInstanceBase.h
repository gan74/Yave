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
#ifndef N_GRAPHICS_SHADERINSTANCEBASE_H
#define N_GRAPHICS_SHADERINSTANCEBASE_H

#include "Shader.h"
#include "ShaderProgram.h"
#include "DynamicBuffer.h"

namespace n {
namespace graphics {

class ShaderLinkingException : public std::exception
{
	public:
		virtual const char *what() const throw() override {
			return m.toChar();
		}

	private:
		friend class ShaderInstanceBase;

		ShaderLinkingException(const core::String &msg) : std::exception(), m(msg) {
		}

		const core::String m;
};

class ShaderInstanceBase : public NonCopyable
{
	class TextureBinding
	{
		public:
			TextureBinding();
			void bind(uint slot) const;

			template<typename T>
			TextureBinding &operator=(const T &t) {
				t.synchronize();
				tex = t.data;
				return *this;
			}

			TextureBinding &operator=(TextureSampler smp) {
				sampler = smp;
				return *this;
			}

		private:
			core::SmartPtr<TextureBase::Data> tex;
			TextureSampler sampler;
	};

	class ImageBinding
	{
		public:
			ImageBinding();
			void bind(uint slot) const;

			template<typename T>
			ImageBinding &operator=(const T &t) {
				t.synchronize();
				tex = t.data;
				format = t.getFormat();
				return *this;
			}

			ImageBinding &operator=(TextureAccess a) {
				access = a;
				return *this;
			}

			ImageBinding &operator=(uint m) {
				mips = m;
				return *this;
			}

		private:
			core::SmartPtr<TextureBase::Data> tex;
			TextureAccess access;
			ImageFormat format;
			uint mips;
	};

	public:
		typedef gl::UniformAddr UniformAddr;

	protected:
		struct UniformInfo
		{
			UniformAddr addr;
			uint size;
			gl::UniformType type;
		};

	public:
		~ShaderInstanceBase();

		static const ShaderInstanceBase *getCurrent();
		static void validateState();

		void setValue(UniformInfo addr, const int *a, uint count) const;
		void setValue(UniformInfo addr, const uint *a, uint count) const;
		void setValue(UniformInfo addr, const float *f, uint count) const;
		void setValue(UniformInfo addr, const math::Vec2i *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec3i *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec4i *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec2 *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec3 *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec4 *v, uint count) const;
		void setValue(UniformInfo addr, const math::Matrix2<float> *m, uint count) const;
		void setValue(UniformInfo addr, const math::Matrix3<float> *m, uint count) const;
		void setValue(UniformInfo addr, const math::Matrix4<float> *m, uint count) const;
		void setValue(UniformInfo slot, const Texture &t, TextureSampler sampler = TextureSampler::Trilinear) const;
		void setValue(UniformInfo slot, const RenderableTexture &t, TextureSampler sampler = TextureSampler::Trilinear) const;
		void setValue(UniformInfo slot, const CubeMap &t, TextureSampler sampler = TextureSampler::Trilinear) const;
		void setValue(UniformInfo info, const RenderableTexture &t, TextureAccess access, uint mip = 0) const;
		void setValue(UniformInfo info, const RenderableCubeMap &t, TextureAccess access, uint mip = 0) const;


		template<typename T>
		void setValue(UniformInfo addr, const T &t) const {
			setValue(addr, &t, 1);
		}

		void setValue(UniformInfo addr, const double &t) const {
			setValue(addr, float(t));
		}

		template<typename T, typename... Args>
		bool setValue(const core::String &name, const T &t, Args... args) const {
			UniformInfo i = getInfo(name);
			if(i.addr != UniformAddr(gl::InvalidIndex)) {
				setValue(i, t, args...);
				return true;
			}
			return false;
		}

		template<typename T, typename... Args>
		bool setValue(ShaderValue v, const T &t, Args... args) const {
			UniformInfo i = getInfo(v);
			if(i.addr != UniformAddr(gl::InvalidIndex)) {
				setValue(i, t, args...);
				return true;
			}
			return false;
		}

		bool setBuffer(const core::String &name, const DynamicBufferBase &buffer) const;
		bool setBuffer(ShaderValue v, const DynamicBufferBase &buffer) const;

		UniformInfo getInfo(const core::String &name) const;
		UniformInfo getInfo(ShaderValue v) const;

	protected:
		ShaderInstanceBase();

		void bind();
		void rebind();
		void unbind();

		void compile(const ShaderBase **bases, uint count);
		bool isCompiled() const;

		gl::Handle handle;

	private:
		friend class ShaderProgram;
		friend class GLContext;

		void bindStandards() const;
		void bindTextures() const;
		void bindBuffers() const;

		void getUniforms();
		UniformAddr computeStandardIndex(const core::String &name);

		TextureBinding *texBindings;
		uint samplerCount;

		ImageBinding *imBindings;
		uint imageCount;

		core::SmartPtr<DynamicBufferBase::Data> *bufferBindings;
		core::Hash<core::String, uint> buffers;

		UniformInfo standards[SVMax];
		core::Hash<core::String, UniformInfo> uniformsInfo;

		static ShaderInstanceBase *current;
};

}
}

#endif // N_GRAPHICS_SHADERINSTANCEBASE_H

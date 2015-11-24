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

#ifndef N_GRAPHICS_SHADERINSTANCE_H
#define N_GRAPHICS_SHADERINSTANCE_H

#include "Shader.h"
#include "ShaderProgram.h"
#include "UniformBuffer.h"

namespace n {
namespace graphics {

class ShaderLinkingException : public std::exception
{
	public:
		virtual const char *what() const throw() override {
			return m.toChar();
		}

	private:
		friend class ShaderInstance;

		ShaderLinkingException(const core::String &msg) : std::exception(), m(msg) {
		}

		const core::String m;
};

class ShaderInstance : NonCopyable
{
	public:
		typedef gl::UniformAddr UniformAddr;

	private:
		struct UniformInfo
		{
			UniformAddr addr;
			uint size;
			gl::UniformType type;
		};

	public:
		ShaderInstance(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom = 0);
		ShaderInstance(const Shader<FragmentShader> *frag, ShaderProgram::StandardVertexShader vert, const Shader<GeometryShader> *geom = 0);
		~ShaderInstance();

		void bind();
		void rebind();
		void unbind();

		static const ShaderInstance *getCurrent();
		static void validateState();

		void setValue(UniformInfo addr, const int *a, uint count) const;
		void setValue(UniformInfo addr, const uint *a, uint count) const;
		void setValue(UniformInfo addr, const float *f, uint count) const;
		void setValue(UniformInfo addr, const math::Vec2i *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec3i *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec2 *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec3 *v, uint count) const;
		void setValue(UniformInfo addr, const math::Vec4 *v, uint count) const;
		void setValue(UniformInfo addr, const math::Matrix2<float> *m, uint count) const;
		void setValue(UniformInfo addr, const math::Matrix3<float> *m, uint count) const;
		void setValue(UniformInfo addr, const math::Matrix4<float> *m, uint count) const;
		void setValue(UniformInfo slot, const Texture &t, TextureSampler sampler = TextureSampler::Default) const;

		template<typename T>
		void setValue(UniformInfo addr, const T &t) const {
			setValue(addr, &t, 1);
		}

		void setValue(UniformInfo addr, const double &t) const {
			setValue(addr, float(t));
		}

		template<typename T, typename... Args>
		void setValue(const core::String &name, const T &t, Args... args) const {
			UniformInfo i = getInfo(name);
			if(i.addr != UniformAddr(gl::InvalidIndex)) {
				setValue(i, t, args...);
			}
		}

		template<typename T, typename... Args>
		void setValue(ShaderValue v, const T &t, Args... args) const {
			setValue(standards[v], t, args...);
		}

		void setBuffer(const core::String &name, const DynamicBufferBase &buffer) const;
		void setBuffer(ShaderValue v, const DynamicBufferBase &buffer) const;

		UniformInfo getInfo(const core::String &name) const;


	private:
		friend class ShaderProgram;
		friend class GLContext;

		void bindStandards() const;
		void bindTextures() const;
		void bindBuffers() const;

		void compile();
		void getUniforms();
		UniformAddr computeStandardIndex(const core::String &name);

		gl::Handle handle;

		uint samplerCount;
		internal::TextureBinding *texBindings;

		core::SmartPtr<DynamicBufferBase::Data> *bufferBindings;
		core::Hash<core::String, uint> buffers;

		UniformInfo standards[SVMax];
		core::Hash<core::String, UniformInfo> uniformsInfo;


		const ShaderBase *bases[3];


		static ShaderInstance *current;
};


}
}

#endif // SHADERINSTANCE_H

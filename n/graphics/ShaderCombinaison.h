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

#ifndef N_GRAPHICS_SHADERCOMBINAISON
#define N_GRAPHICS_SHADERCOMBINAISON

#include "Shader.h"
#include "UniformBuffer.h"
#include "GLContext.h"
#include "Texture.h"
#include "CubeMap.h"
#include "TextureBinding.h"
#include <n/core/Map.h>
#include <n/core/Hash.h>
#include <n/math/Matrix.h>

namespace n {
namespace graphics {

class ShaderLinkingException : public std::exception
{
	public:
		virtual const char *what() const throw() override {
			return m.toChar();
		}

	private:
		friend class ShaderCombinaison;
		ShaderLinkingException(const core::String &msg) : std::exception(), m(msg) {
		}

		const core::String m;
};

class ShaderCombinaison : core::NonCopyable
{
	public:
		typedef gl::GLint UniformAddr;

	private:
		struct UniformInfo
		{
			UniformAddr addr;
			uint size;
		};

		struct BlockInfo
		{
			gl::GLuint addr;
			gl::GLuint slot;
			const typename internal::DynamicBufferBase<GL_UNIFORM_BUFFER> *buffer;
		};


	public:
		enum StandardValue
		{
			Color,
			Metallic,
			DiffuseMap,
			DiffuseMul,
			NormalMap,
			NormalMul,
			RoughnessMap,
			RoughnessMul,
			Texture0,
			Texture1,
			Texture2,
			Texture3,
			Max
		};

		static const char *StandardValueName[Max];

		class Uniform : core::NonCopyable
		{
			public:
				Uniform(Uniform &&u) : sh(u.sh), name(u.name) {
				}

				template<typename T>
				T operator=(const T &t) {
					sh->setValue(name, t);
					return t;
				}

				template<typename T>
				const UniformBuffer<T> &operator=(const UniformBuffer<T> &t) {
					sh->setBuffer(name, &t);
					return t;
				}

			private:
				friend class ShaderCombinaison;
				Uniform(const ShaderCombinaison *s, const core::String &a) : sh(s), name(a) {
				}

				const ShaderCombinaison *sh;
				core::String name;
		};

		ShaderCombinaison(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom = 0);
		ShaderCombinaison(const Shader<FragmentShader> *frag, ShaderProgram::StandardVertexShader vs);
		~ShaderCombinaison();

		bool isValid() const {
			return handle;
		}

		const core::String &getLogs() const {
			return logs;
		}

		void bind();

		void unbind() {
			if(GLContext::getContext()->shader != this) {
				return;
			}
			ShaderProgram(ShaderProgram::getNullProgram()).bind();
		}

		UniformAddr getAddr(const core::String &name) const {
			return getInfo(name).addr;
		}

		Uniform operator[](const core::String &name) const {
			return Uniform(this, name);
		}

		template<typename T, typename... Args>
		void setValue(const core::String &name, const T &t, Args... args) const {
			UniformAddr i = getAddr(name);
			if(i != UniformAddr(GL_INVALID_INDEX)) {
				setValue(i, t, args...);
			}
		}

		template<typename T, typename... Args>
		void setValue(StandardValue val, const T &t, Args... args) const {
			UniformAddr i = standards[val];
			if(i != UniformAddr(GL_INVALID_INDEX)) {
				setValue(i, t, args...);
			}
		}

		void setValue(UniformAddr addr, int a) const {
			gl::glProgramUniform1i(handle, addr, a);
		}

		void setValue(UniformAddr addr, uint a) const {
			gl::glProgramUniform1ui(handle, addr, a);
		}

		void setValue(UniformAddr addr, float f) const {
			gl::glProgramUniform1f(handle, addr, f);
		}

		void setValue(UniformAddr addr, double f) const {
			gl::glProgramUniform1f(handle, addr, f);
		}

		void setValue(UniformAddr addr, const math::Vec2i &v) const {
			gl::glProgramUniform2iv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec3i &v) const {
			gl::glProgramUniform3iv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec2 &v) const {
			gl::glProgramUniform2fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec3 &v) const {
			gl::glProgramUniform3fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec4 &v) const {
			gl::glProgramUniform4fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Matrix2<float> &m) const {
			gl::glProgramUniformMatrix2fv(handle, addr, 1, GL_TRUE, m.begin());
		}

		void setValue(UniformAddr addr, const math::Matrix3<float> &m) const {
			gl::glProgramUniformMatrix3fv(handle, addr, 1, GL_TRUE, m.begin());
		}

		void setValue(UniformAddr addr, const math::Matrix4<float> &m) const {
			gl::glProgramUniformMatrix4fv(handle, addr, 1, GL_TRUE, m.begin());
		}

		void setValue(UniformAddr slot, const Texture &t, TextureSampler sampler = TextureSampler::Default) const {
			if(slot != UniformAddr(GL_INVALID_INDEX)) {
				bindings[slot] = t;
				bindings[slot] = sampler;
				if(isCurrent()) {
					bindings[slot].bind(slot);
				} else {
					t.prepare();
				}
			}
		}

		void setValue(UniformAddr slot, const CubeMap &t, TextureSampler sampler = TextureSampler::Default) const {
			if(slot != UniformAddr(GL_INVALID_INDEX)) {
				bindings[slot] = t;
				bindings[slot] = sampler;
				if(isCurrent()) {
					bindings[slot].bind(slot);
				} else {
					t.prepare();
				}
			}
		}

		template<typename T>
		void setBuffer(const core::String &name, const UniformBuffer<T> *buffer) const {
			core::Map<core::String, BlockInfo>::iterator it = blocks.find(name);
			BlockInfo infos{gl::GLuint(GL_INVALID_INDEX), 0, 0};
			if(it == blocks.end()) {
				gl::GLint index = gl::glGetUniformBlockIndex(handle, name.toChar());
				static uint bs = 0;
				blocks[name] = infos = BlockInfo{gl::GLuint(index), bs++, 0};
				gl::glUniformBlockBinding(handle, infos.addr, infos.slot);
			} else {
				infos = (*it)._2;
			}
			(*it)._2.buffer = buffer;
			if(buffer) {
				if(isCurrent() && infos.addr != gl::GLuint(GL_INVALID_INDEX)) {
					buffer->update();
					gl::glBindBufferBase(GL_UNIFORM_BUFFER, infos.slot, buffer->handle);
				}
			}

		}

		#ifdef N_SHADER_SRC
		core::String getShaderSrc(ShaderType i) const {
			return shaders[i] ? shaders[i]->getSrc() : "";
		}
		#endif

	private:
		bool isCurrent() const {
			return GLContext::getContext()->shader == this;
		}

		void compile();
		UniformAddr computeStandardIndex(const core::String &name);
		void getUniforms();

		UniformInfo getInfo(const core::String &name) const {
			return uniformsInfo.get(name, UniformInfo{UniformAddr(GL_INVALID_INDEX), 0});
		}

		const internal::ShaderBase *shaders[3];
		gl::GLuint handle;
		core::String logs;
		core::Hash<core::String, UniformInfo> uniformsInfo;
		UniformAddr standards[Max];
		uint samplerCount;
		internal::TextureBinding *bindings;
		core::Array<gl::GLint> outputs;
		mutable core::Map<core::String, BlockInfo> blocks;

};

}
}

#endif // N_GRAPHICS_SHADERCOMBINAISON


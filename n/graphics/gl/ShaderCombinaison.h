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

#ifndef N_GRAPHICS_GL_SHADERCOMBINAISON
#define N_GRAPHICS_GL_SHADERCOMBINAISON

#include "Shader.h"
#include "UniformBuffer.h"
#include "Texture.h"
#include <n/core/Map.h>
#include <n/math/Matrix.h>
#include <n/defines.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

class ShaderCombinaison
{
	public:
		typedef GLint UniformAddr;

	private:
		struct UniformInfo
		{
			UniformAddr addr;
			uint size;
		};

		struct BlockInfo
		{
			GLuint addr;
			GLuint slot;
			const internal::UniformBufferBase *buffer;
		};

	public:
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
				Uniform(ShaderCombinaison *s, const core::String &a) : sh(s), name(a) {
				}

				ShaderCombinaison *sh;
				core::String name;
		};

		ShaderCombinaison(Shader<FragmentShader> *frag, Shader<VertexShader> *vert = 0, Shader<GeometryShader> *geom = 0) : handle(0) {
			shaders[FragmentShader] = frag;
			shaders[VertexShader] = vert;
			shaders[GeometryShader] = geom;
			compile();
		}

		bool isValid() const {
			return handle;
		}

		const core::String &getLogs() const {
			return logs;
		}

		void bind() const {
			glUseProgram(handle);
			for(const core::Pair<const core::String, BlockInfo> &i : blocks) {
				const BlockInfo &infos = i._2;
				if(infos.buffer && infos.addr != GLuint(GL_INVALID_INDEX)) {
					infos.buffer->update();
					glBindBufferBase(GL_UNIFORM_BUFFER, infos.slot, infos.buffer->handle);
				}
			}
		}

		UniformAddr getAddr(const core::String &name) {
			return getInfo(name).addr;
		}

		Uniform operator[](const core::String &name) {
			return Uniform(this, name);
		}

		template<typename T>
		void setValue(const core::String &name, const T &t) {
			UniformAddr i = getAddr(name);
			if(i != UniformAddr(GL_INVALID_INDEX)) {
				setValue(i, t);
			}
		}

		void setValue(UniformAddr addr, int a) {
			glProgramUniform1i(handle, addr, a);
		}

		void setValue(UniformAddr addr, uint a) {
			glProgramUniform1ui(handle, addr, a);
		}

		void setValue(UniformAddr addr, float f) {
			glProgramUniform1f(handle, addr, f);
		}

		void setValue(UniformAddr addr, const math::Vec2i &v) {
			glProgramUniform2iv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec3i &v) {
			glProgramUniform3iv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec2 &v) {
			glProgramUniform2fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec3 &v) {
			glProgramUniform3fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Vec4 &v) {
			glProgramUniform4fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Matrix4 &m) {
			glProgramUniformMatrix4fv(handle, addr, 1, GL_TRUE, m.begin());
		}

		void setValue(UniformAddr addr, const Texture &t) {
			setValue(addr, t.data ? t.data->handle : 0);
		}

		template<typename T>
		void setBuffer(const core::String &name, const UniformBuffer<T> *buffer) {
			core::Map<core::String, BlockInfo>::iterator it = blocks.find(name);
			BlockInfo infos{GLuint(GL_INVALID_INDEX), 0, 0};
			if(it == blocks.end()) {
				GLint index = glGetUniformBlockIndex(handle, name.toChar());
				static uint bs = 0;
				blocks[name] = infos = BlockInfo{GLuint(index), bs++, 0};
				glUniformBlockBinding(handle, infos.addr, infos.slot);
			} else {
				infos = (*it)._2;
			}
			(*it)._2.buffer = buffer;
			if(buffer) {
				if(isCurrent() && infos.addr != GLuint(GL_INVALID_INDEX)) {
					buffer->update();
					glBindBufferBase(GL_UNIFORM_BUFFER, infos.slot, buffer->handle);
				}
			}

		}

	private:
		bool isCurrent() const {
			return true;
		}

		void compile() {
			handle = glCreateProgram();
			bool val = true;
			for(uint i = 0; i != 3; i++) {
				if(shaders[i]) {
					glAttachShader(handle, shaders[i]->handle);
					val &= shaders[i]->isValid();
				}
			}
			glLinkProgram(handle);
			int res = 0;
			glGetProgramiv(handle, GL_LINK_STATUS, &res);
			if(!res || !val) {
				int size = 0;
				glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &size);
				char *msg = new char[size + 1];
				glGetProgramInfoLog(handle, size, &res, msg);
				glDeleteProgram(handle);
				handle = 0;
				msg[size] = '\0';
				logs = msg;
				delete[] msg;
			} else {
				getUniforms();
			}
		}

		void getUniforms() {
			const uint max = 512;
			char name[max];
			int uniforms = 0;
			glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniforms);
			for(uint i = 0; i != (uint)uniforms; i++) {
				int size = 0;
				GLenum type = GL_NONE;
				glGetActiveUniform(handle, i, max, 0, &size, &type, name);
				core::String uniform = name;
				if(uniform.endWith("[0]")) {
					uniform = uniform.subString(0, uniform.size() - 3);
				}
				uniformsInfo[uniform] = UniformInfo({glGetUniformLocation(handle, name), (uint)size});
			}
		}

		UniformInfo getInfo(const core::String &name) {
			return uniformsInfo.get(name, UniformInfo{UniformAddr(GL_INVALID_INDEX), 0});
		}

		internal::ShaderBase *shaders[3];
		GLuint handle;
		core::String logs;
		core::Map<core::String, UniformInfo> uniformsInfo;
		core::Map<core::String, BlockInfo> blocks;
};

}
}
}

#endif

#endif // N_GRAPHICS_GL_SHADERCOMBINAISON


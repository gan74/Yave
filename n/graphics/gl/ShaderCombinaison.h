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

	public:
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
		}

		void setValue(UniformAddr addr, int a) {
			glProgramUniform1iv(handle, addr, 1, &a);
		}

		void setValue(UniformAddr addr, float f) {
			glProgramUniform1fv(handle, addr, 1, &f);
		}

		void setValue(UniformAddr addr, math::Vec2i v) {
			glProgramUniform2iv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, math::Vec3i v) {
			glProgramUniform3iv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, math::Vec2 v) {
			glProgramUniform2fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, math::Vec3 v) {
			glProgramUniform3fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, math::Vec4 v) {
			glProgramUniform4fv(handle, addr, 1, v.begin());
		}

		void setValue(UniformAddr addr, const math::Matrix4 &m) {
			glProgramUniformMatrix4fv(handle, addr, 1, GL_TRUE, m.begin());
		}

	private:
		void compile() {
			handle = glCreateProgram();
			for(uint i = 0; i != 3; i++) {
				if(shaders[i]) {
					glAttachShader(handle, shaders[i]->handle);
				}
			}
			glLinkProgram(handle);
			int res = 0;
			glGetProgramiv(handle, GL_LINK_STATUS, &res);
			if(!res) {
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

		internal::ShaderBase *shaders[3];
		GLuint handle;
		core::String logs;
		core::Map<core::String, UniformInfo> uniformsInfo;
};

}
}
}

#endif

#endif // N_GRAPHICS_GL_SHADERCOMBINAISON


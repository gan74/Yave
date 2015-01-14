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

#ifndef N_GRAPHICS_GL_SHADER
#define N_GRAPHICS_GL_SHADER

#include <n/core/String.h>
#include "GL.h"
#include <n/defines.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

class ShaderCombinaison;

namespace internal {

class ShaderBase
{
	public:
		const core::String &getLogs() const {
			return logs;
		}

		bool isValid() {
			return handle;
		}

	protected:
		friend class gl::ShaderCombinaison;

		ShaderBase() : handle(0) {
		}

		~ShaderBase() {
			if(handle) {
				glDeleteShader(handle);
			}
		}

		GLuint handle;
		core::String logs;
};

}

enum ShaderType
{
	FragmentShader = 0,
	VertexShader = 1,
	GeometryShader = 2
};

template<ShaderType Type>
class Shader : core::NonCopyable, public internal::ShaderBase
{
	public:
		Shader(const core::String &src) : internal::ShaderBase() {
			load(src);
		}

		~Shader() {
		}

	private:
		void load(const core::String &src) {
			GLenum glType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER};
			handle = glCreateShader(glType[Type]);
			const char *str = src.toChar();
			glShaderSource(handle, 1, &str, 0);
			glCompileShader(handle);
			int res = 0;
			glGetShaderiv(handle, GL_COMPILE_STATUS, &res);
			if(!res) {
				int size = 0;
				glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &size);
				char *msg = new char[size + 1];
				glGetShaderInfoLog(handle, size, &size, msg);
				glDeleteShader(handle);
				handle = 0;
				msg[size] = '\0';
				logs = msg;
				delete msg;
			}
		}

};


}
}
}

#endif

#endif // N_GRAPHICS_GL_SHADER


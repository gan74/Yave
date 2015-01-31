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

#ifndef N_GRAPHICS_SHADER
#define N_GRAPHICS_SHADER

#include <n/core/String.h>
#include "GL.h"

namespace n {
namespace graphics {

class ShaderCombinaison;

namespace internal {

class ShaderBase : core::NonCopyable
{
	public:
		const core::String &getLogs() const {
			return logs;
		}

		bool isValid() {
			return handle;
		}

	protected:
		friend class graphics::ShaderCombinaison;

		ShaderBase() : handle(0) {
		}

		~ShaderBase() {
			if(handle) {
				GLContext::getContext()->addGLTask([=]() { gl::glDeleteShader(handle); });
			}
		}

		gl::GLuint handle;
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
class Shader : public internal::ShaderBase
{
	public:
		Shader(const core::String &src) : internal::ShaderBase() {
			load(src);
		}

		~Shader() {
		}

	private:
		void load(const core::String &src) {
			gl::GLenum glType[] = {GL_FRAGMENT_SHADER, GL_VERTEX_SHADER, GL_GEOMETRY_SHADER};
			handle = gl::glCreateShader(glType[Type]);
			const char *str = src.toChar();
			gl::glShaderSource(handle, 1, &str, 0);
			gl::glCompileShader(handle);
			int res = 0;
			gl::glGetShaderiv(handle, GL_COMPILE_STATUS, &res);
			if(!res) {
				int size = 0;
				gl::glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &size);
				char *msg = new char[size + 1];
				gl::glGetShaderInfoLog(handle, size, &size, msg);
				gl::glDeleteShader(handle);
				handle = 0;
				msg[size] = '\0';
				logs = msg;
				delete msg;
			}
		}

};


}
}

#endif // N_GRAPHICS_SHADER


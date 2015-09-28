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

#ifndef N_GRAPHICS_SHADERPROGRAM_H
#define N_GRAPHICS_SHADERPROGRAM_H

#include "Shader.h"
#include "TextureBinding.h"

namespace n {
namespace graphics {

namespace internal {
	struct ShaderProgramData
	{
		enum NullPolicy
		{
			Keep,
			Replace
		};

		gl::GLuint handle;
		ShaderBase *bases[3];
		NullPolicy policies[3];


		~ShaderProgramData() {
			if(handle) {
				gl::GLuint h = handle;
				GLContext::getContext()->addGLTask([=]() { gl::glDeleteProgramPipelines(1, &h); });
			}
		}
	};

}

class ShaderProgram
{
	typedef internal::ShaderProgramData Data;
	public:
		enum StandardVertexShader
		{
			ProjectionShader = 0,
			NoProjectionShader = 1
		};


		ShaderProgram(Shader<FragmentShader> *frag, Shader<VertexShader> *vert, Shader<GeometryShader> *geom = 0);
		ShaderProgram(Shader<FragmentShader> *frag, StandardVertexShader std = ProjectionShader, Shader<GeometryShader> *geom = 0);
		ShaderProgram();
		~ShaderProgram();

		void bind() const;
		void unbind() const;
		void rebind() const;

		bool operator==(const ShaderProgram &p) const;
		bool operator!=(const ShaderProgram &p) const;
		bool operator<(const ShaderProgram &p) const;

		template<typename... Args>
		void setValue(Args... args) const {
			if(isCurrent()) {
				for(uint i = 0; i != 3; i++) {
					if(bound[i]) {
						bound[i]->setValue(args...);
					}
				}
			} else {
				for(uint i = 0; i != 3; i++) {
					if(data->bases[i]) {
						data->bases[i]->setValue(args...);
					}
				}
			}
		}


		bool isCurrent() const;


		static void bindStandards();

	private:
		friend class GLContext;

		ShaderProgram(core::SmartPtr<Data> ptr);

		void setup() const;

		core::SmartPtr<Data> data;

		static core::SmartPtr<ShaderProgram::Data> nullData;
		static ShaderBase *bound[3];
};



}
}

#endif // N_GRAPHICS_SHADERPROGRAM_H

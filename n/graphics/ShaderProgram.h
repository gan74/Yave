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

#ifndef N_GRAPHICS_SHADERPROGRAM
#define N_GRAPHICS_SHADERPROGRAM

#include <n/core/Map.h>
#include <n/core/SmartPtr.h>

namespace n {
namespace graphics {

enum ShaderType
{
	FragmentShader = 0,
	VertexShader = 1,
	GeometryShader = 2
};


template<ShaderType Type>
class Shader;

class ShaderCombinaison;


namespace internal {
	struct ShaderProgramCombinaison
	{
		Shader<FragmentShader> *frag;
		Shader<VertexShader> *vert;
		Shader<GeometryShader> *geom;

		bool operator==(const ShaderProgramCombinaison &c) const {
			return frag == c.frag && vert == c.vert && geom == c.geom;
		}

		bool operator!=(const ShaderProgramCombinaison &c) const {
			return !operator==(c);
		}

		bool operator<(const ShaderProgramCombinaison &c) const {
			if(frag != c.frag) {
				return frag < c.frag;
			}
			if(vert != c.vert) {
				return vert < c.vert;
			}
			return geom < c.geom;
		}
	};

	struct ShaderProgram
	{
		ShaderProgram(Shader<FragmentShader> *frag, Shader<VertexShader> *vert, Shader<GeometryShader> *geom);
		~ShaderProgram();

		ShaderProgramCombinaison getCombinaison() const;

		ShaderProgramCombinaison base;
		core::Map<ShaderProgramCombinaison, ShaderCombinaison *> shaders;
	};
}

class ShaderProgram
{
	typedef core::SmartPtr<internal::ShaderProgram> ShaderProgramPtr;
	public:

		enum StandardVertexShader
		{
			ProjectionShader = 0,
			NoProjectionShader = 1
		};

		ShaderProgram(Shader<FragmentShader> *frag, Shader<VertexShader> *vert = 0, Shader<GeometryShader> *geom = 0);
		ShaderProgram(Shader<FragmentShader> *frag, StandardVertexShader vert, Shader<GeometryShader> *geom = 0);

		void bind();

		bool isActive() const;

		static void setDefaultShader(Shader<FragmentShader> *s);
		static void setDefaultShader(Shader<VertexShader> *s);
		static void setDefaultShader(Shader<GeometryShader> *s);

		static bool isDefaultShader(Shader<FragmentShader> *s);
		static bool isDefaultShader(Shader<VertexShader> *s);
		static bool isDefaultShader(Shader<GeometryShader> *s);


		static Shader<VertexShader> *getDefaultVertexShader(StandardVertexShader type = ProjectionShader);
		static Shader<FragmentShader> *getDefaultFragmentShader();

	private:
		friend class GLContext;
		friend class ShaderCombinaison;

		static const ShaderProgramPtr &getNullProgram();
		static void rebind();

		ShaderProgram(const ShaderProgramPtr &p) : ptr(p) {
		}

		ShaderProgramPtr ptr;
};

}
}

#endif // N_GRAPHICS_SHADERPROGRAM


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
		union
		{
			struct
			{
				const Shader<FragmentShader> *frag;
				const Shader<VertexShader> *vert;
				const Shader<GeometryShader> *geom;
			} shaders;
			const void *ptrs[3];
		};


		bool operator==(const ShaderProgramCombinaison &c) const {
			for(uint i = 0; i != 3; i++) {
				if(ptrs[i] != c.ptrs[i]) {
					return false;
				}
			}
			return true;
		}

		bool operator!=(const ShaderProgramCombinaison &c) const {
			return !operator==(c);
		}

		bool operator<(const ShaderProgramCombinaison &c) const {
			for(uint i = 0; i != 3; i++) {
				if(ptrs[i] != c.ptrs[i]) {
					return ptrs[i] < c.ptrs[i];
				}
			}
			return false;
		}

		bool operator>(const ShaderProgramCombinaison &c) const {
			for(uint i = 0; i != 3; i++) {
				if(ptrs[i] != c.ptrs[i]) {
					return ptrs[i] > c.ptrs[i];
				}
			}
			return false;
		}

		bool isNull() const {
			for(uint i = 0; i != 3; i++) {
				if(ptrs[i]) {
					return false;
				}
			}
			return false;
		}
	};

	struct ShaderProgram : core::NonCopyable
	{
		ShaderProgram(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom);
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

		ShaderProgram();
		ShaderProgram(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert = 0, const Shader<GeometryShader> *geom = 0);
		ShaderProgram(const Shader<FragmentShader> *frag, StandardVertexShader vert, const Shader<GeometryShader> *geom = 0);

		const ShaderCombinaison *bind() const;

		bool isActive() const;

		static void setDefaultShader(const Shader<FragmentShader> *s);
		static void setDefaultShader(const Shader<VertexShader> *s);
		static void setDefaultShader(const Shader<GeometryShader> *s);

		static bool isDefaultShader(const Shader<FragmentShader> *s);
		static bool isDefaultShader(const Shader<VertexShader> *s);
		static bool isDefaultShader(const Shader<GeometryShader> *s);


		static Shader<VertexShader> *getStandardVertexShader(StandardVertexShader type = ProjectionShader);
		static Shader<FragmentShader> *getStandardFragmentShader();

		bool isDefaultProgram() const {
			return ptr->base.isNull();
		}

		bool operator==(const ShaderProgram &p) const {
			return ptr->base == p.ptr->base;
		}

		bool operator!=(const ShaderProgram &p) const {
			return ptr->base != p.ptr->base;
		}

		bool operator<(const ShaderProgram &p) const {
			return ptr->base < p.ptr->base;
		}

		bool operator>(const ShaderProgram &p) const {
			return ptr->base > p.ptr->base;
		}

		template<ShaderType Type>
		const Shader<Type> *getShader() const {
			return reinterpret_cast<const Shader<Type> *>(ptr->base.ptrs[Type]);
		}

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


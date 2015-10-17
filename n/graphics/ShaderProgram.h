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
		const ShaderBase *bases[3];
	};
}


class ShaderProgram
{
	typedef internal::ShaderProgramData Data;


	/*class UniformBase : NonCopyable
	{
		public:
			virtual ~UniformBase() {
			}

			virtual void set(ShaderInstance *inst) = 0;
	};



	template<typename U, typename T>
	class Uniform : public UniformBase
	{
		public:
			Uniform(U u, T t) : name(u), data(t) {
			}

			virtual void set(ShaderInstance *inst) override {
				inst->setValue(name, data);
			}

			U name;
			T data;
	};*/

	public:
		enum StandardVertexShader
		{
			ProjectionShader = 0,
			NoProjectionShader = 1
		};

		static Shader<VertexShader> *getStandardVertexShader(StandardVertexShader std = ShaderProgram::ProjectionShader);
		static Shader<FragmentShader> *getStandardFragmentShader();


		ShaderProgram(const Shader<FragmentShader> *frag, const Shader<VertexShader> *vert, const Shader<GeometryShader> *geom = 0);
		ShaderProgram(const Shader<FragmentShader> *frag, StandardVertexShader std = ProjectionShader, const Shader<GeometryShader> *geom = 0);
		ShaderProgram();
		~ShaderProgram();


		const ShaderInstance *bind() const;
		const ShaderInstance *rebind() const;
		void unbind() const;

		bool operator==(const ShaderProgram &p) const;
		bool operator!=(const ShaderProgram &p) const;
		bool operator<(const ShaderProgram &p) const;

		bool isCurrent() const;

		template<ShaderType Type>
		const Shader<Type> *getShader() const {
			return static_cast<const Shader<Type> *>(data->bases[Type]);
		}

	private:
		friend class GLContext;

		ShaderProgram(core::SmartPtr<Data> ptr);

		core::SmartPtr<Data> data;

		//mutable core::Array<UniformBase *> uniforms;

		static core::SmartPtr<ShaderProgram::Data> nullData;
};



}
}

#endif // N_GRAPHICS_SHADERPROGRAM_H

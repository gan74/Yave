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

#include <n/types.h>
#include <n/utils.h>
#include <n/core/String.h>
#include <n/core/Hash.h>
#include <exception>
#include "GLContext.h"
#include "Texture.h"
#include "CubeMap.h"
#include "TextureBinding.h"
#include "GL.h"

namespace n {
namespace graphics {

class ShaderProgram;
class ShaderInstance;
class ShaderBase;

enum ShaderValue
{
	SVColor,
	SVMetallic,
	SVDiffuseMap,
	SVDiffuseMul,
	SVNormalMap,
	SVNormalMul,
	SVRoughnessMap,
	SVRoughnessMul,
	SVTexture0,
	SVTexture1,
	SVTexture2,
	SVTexture3,
	SVUniformBuffer,
	SVModelMatrixBuffer,
	SVBaseInstance,
	SVProjMatrix,
	SVViewMatrix,
	SVViewportSize,
	SVViewProjMatrix,

	SVMax
};


namespace internal {
	void rebindProgram();
}

class ShaderCompilationException : public std::exception
{
	public:
		virtual const char *what() const throw() override {
			return m.toChar();
		}

	private:
		template<ShaderType Type>
		friend class Shader;
		friend class ShaderBase;

		ShaderCompilationException(const core::String &msg) : std::exception(), m(msg) {
		}

		const core::String m;
};

class ShaderBase : NonCopyable
{
	public:
		~ShaderBase();

		const core::String &getLogs() const;
		bool isValid() const;
		uint getVersion() const;
		bool isCurrent() const;
		ShaderType getType() const;

		#ifdef N_SHADER_SRC
		const core::String &getSrc() const {
			return source;
		}
		#endif

	protected:
		friend class graphics::ShaderInstance;
		friend class graphics::ShaderProgram;

		ShaderBase(ShaderType t);

		uint load(core::String src, uint vers);
		core::String parse(core::String src, uint vers);
		ShaderType type;
		uint version;
		gl::Handle handle;

		core::String logs;

		#ifdef N_SHADER_SRC
		core::String source;
		#endif


		static ShaderBase *currents[3];
};


template<ShaderType Type>
class Shader : public ShaderBase
{

	public:
		Shader(const core::String &src, uint vers = 420) : ShaderBase(Type) {
			version = load(src, vers);
		}

		static void setDefault(Shader<Type> *t) {
			if(currents[Type] != t) {
				currents[Type] = t;
				internal::rebindProgram();
			}
		}

		void bindAsDefault() {
			setDefault(this);
		}


};

}
}

#endif // N_GRAPHICS_SHADER


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
class ShaderBase;

enum ShaderType
{
	FragmentShader,
	VertexShader,
	GeometryShader
};

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
	SVMax
};

static const char *ShaderValueName[ShaderValue::SVMax] = {
	"n_Color",
	"n_Metallic",
	"n_DiffuseMap",
	"n_DiffuseMul",
	"n_NormalMap",
	"n_NormalMul",
	"n_RoughnessMap",
	"n_RoughnessMul",
	"n_0",
	"n_1",
	"n_2",
	"n_3",
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

class ShaderBase : core::NonCopyable
{
	public:
		typedef gl::GLint UniformAddr;

	private:
		struct UniformInfo
		{
			UniformAddr addr;
			uint size;
		};

	public:
		~ShaderBase();

		const core::String &getLogs() const;
		bool isValid() const;
		uint getVersion() const;
		bool isCurrent() const;
		ShaderType getType() const;
		UniformAddr getAddr(const core::String &name) const;

		template<typename T, typename... Args>
		bool setValue(const core::String &name, const T &t, Args... args) const {
			UniformAddr i = getAddr(name);
			if(i != UniformAddr(GL_INVALID_INDEX)) {
				setValue(i, t, args...);
				return true;
			}
			return false;
		}

		template<typename T, typename... Args>
		bool setValue(ShaderValue v, const T &t, Args... args) const {
			return setValue(ShaderValueName[v], t, args...);
		}

		void setValue(UniformAddr addr, int a) const;
		void setValue(UniformAddr addr, uint a) const;
		void setValue(UniformAddr addr, float f) const;
		void setValue(UniformAddr addr, double f) const;
		void setValue(UniformAddr addr, const math::Vec2i &v) const;
		void setValue(UniformAddr addr, const math::Vec3i &v) const;
		void setValue(UniformAddr addr, const math::Vec2 &v) const;
		void setValue(UniformAddr addr, const math::Vec3 &v) const;
		void setValue(UniformAddr addr, const math::Vec4 &v) const;
		void setValue(UniformAddr addr, const math::Matrix2<float> &m) const;
		void setValue(UniformAddr addr, const math::Matrix3<float> &m) const;
		void setValue(UniformAddr addr, const math::Matrix4<float> &m) const;
		void setValue(UniformAddr slot, const Texture &t, TextureSampler sampler = TextureSampler::Default) const;

		#ifdef N_SHADER_SRC
		const core::String &getSrc() const {
			return source;
		}
		#endif


		void bindStandards();

	protected:
		friend class graphics::ShaderProgram;

		ShaderBase(ShaderType t);

		UniformInfo getInfo(const core::String &name) const;
		uint load(core::String src, uint vers);
		core::String parse(core::String src, uint vers);
		void getUniforms();

		ShaderType type;
		uint version;
		gl::GLuint handle;
		core::Hash<core::String, UniformInfo> uniformsInfo;
		uint samplerCount;
		internal::TextureBinding *bindings;

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


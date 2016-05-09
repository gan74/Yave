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

#include "IBLProbe.h"
#include "ShaderInstance.h"
#include "CubeFrameBuffer.h"
#include "VertexArrayObject.h"
#include "DeferredCommon.h"

namespace n {
namespace graphics {


static ShaderInstance *getShader() {
	static ShaderInstance *inst = 0;
	if(!inst) {
		inst = new ShaderInstance(new Shader<FragmentShader>(
			"uniform samplerCube n_Cube;"
			"uniform float roughness;"

			"layout(location = 0) out vec4 n_0;"
			"layout(location = 1) out vec4 n_1;"
			"layout(location = 2) out vec4 n_2;"
			"layout(location = 3) out vec4 n_3;"
			"layout(location = 4) out vec4 n_4;"
			"layout(location = 5) out vec4 n_5;"

			+ getBRDFs() +

			"vec3 normal(uint side) {"
				"vec2 tex = n_TexCoord * 2.0 - 1.0;"
				"if(side == 0) return vec3(tex, 1.0);" // top
				"else if(side == 1) return vec3(-tex.x, tex.y, -1.0);" // bottom"
				"else if(side == 2) return vec3(tex.x, -1.0, tex.y);" // left
				"else if(side == 3) return vec3(tex.x, 1.0, -tex.y);" // right
				"else if(side == 4) return vec3(1.0, tex.y, -tex.x);" // front
				"return vec3(-1.0, tex.y, tex.x);" // back
			"}"

			"mat3 genWorld(vec3 Z) {"
				"vec3 U = abs(Z.z) > 0.999 ? vec3(1, 0, 0) : vec3(0, 0, -1);"
				"vec3 X = normalize(cross(Z, U));"
				"vec3 Y = cross(X, Z);"
				"return mat3(X, Y, Z);"
			"}"

			"vec4 filterEnv(vec3 R) {"
					"R = normalize(R);"

					"vec3 N = R;"
					"vec3 V = R;"
					"mat3 W = genWorld(R);"

					"float sum = 0.0;"
					"vec4 color = vec4(0.0);"

					"const uint samples = 1024;"
					"for(uint i = 0; i != samples; i++) {"
						"vec2 Xi = hammersley(i, samples);"
						"vec3 H = normalize(W * brdf_importance_sample(Xi, roughness));"
						"vec3 L = reflect(-V, H);"
						"float NoL = saturate(dot(N, L));"
						"if(NoL > 0.0) {"
							"color += textureLod(n_Cube, L, 0.0) * NoL;"
							"sum += NoL;"
						"}"
					"}"
					"return color / sum;"
				"}"

				"void main() {"
					//"vec3 T = vec3(n_TexCoord, 1.0);"
					"n_0 = filterEnv(normal(0));"
					"n_1 = filterEnv(normal(1));"
					"n_2 = filterEnv(normal(2));"
					"n_3 = filterEnv(normal(3));"
					"n_4 = filterEnv(normal(4));"
					"n_5 = filterEnv(normal(5));"
				"}"),
				ShaderProgram::NoProjectionShader);
	}
	return inst;
}

IBLProbe::IBLProbe(const CubeMap &env) : cube(env), convoluted{&cube, 0, 0, 0, 0, 0, 0} {
}

const CubeMap &IBLProbe::getConvolution(uint index) {
	const float rough[LevelCount] = {0, 0.05, 0.13, 0.25, 0.45, 0.66, 1};

	if(convoluted[index]) {
		return *convoluted[index];
	}
	if(!cube.synchronize(true)) {
		return cube;
	}

	const FrameBufferBase *fb = GLContext::getContext()->getFrameBuffer();

	CubeFrameBuffer cbo(cube.getSize().x(), cube.getFormat());
	cbo.bind();
	ShaderInstance *sh = getShader();
	sh->bind();
	sh->setValue("roughness", rough[index]);
	sh->setValue("n_Cube", cube);
	GLContext::getContext()->getScreen().draw(MaterialRenderData());

	convoluted[index] = new CubeMap(cbo.getAttachment());

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}

	return *convoluted[index];
}

}
}

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
#include "ComputeShaderInstance.h"

namespace n {
namespace graphics {


static ComputeShaderInstance *getShader() {
	static ComputeShaderInstance *inst = 0;
	if(!inst) {
		inst = new ComputeShaderInstance(new Shader<ComputeShader>(
				"layout(local_size_x = 16, local_size_y = 16) in;"
				"layout(rgba8) uniform writeonly volatile imageCube n_Out;"

				"uniform samplerCube n_Cube;"
				"uniform float roughness;"

				+ getBRDFs() +

				"vec3 normal(vec2 texCoord, uint side) {"
					"vec2 tex = texCoord * 2.0 - 1.0;"
					"if(side == 4) return vec3(tex, 1.0);" // top
					"else if(side == 5) return vec3(-tex.x, tex.y, -1.0);" // bottom"
					"else if(side == 2) return vec3(tex.x, -1.0, tex.y);" // left
					"else if(side == 3) return vec3(tex.x, 1.0, -tex.y);" // right
					"else if(side == 0) return vec3(1.0, tex.y, -tex.x);" // front
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
					"ivec2 coord = ivec2(gl_GlobalInvocationID.xy);"
					"vec2 uv = gl_GlobalInvocationID.xy / vec2(imageSize(n_Out));"
					"for(int i = 0; i != 6; i++) {"
						"imageStore(n_Out, ivec3(coord, i), filterEnv(normal(uv, i)));"
						//"imageStore(n_Out, ivec3(coord, i), vec4(0, 1, 0, 1));"
					"}"

				"}"));
	}
	return inst;
}

IBLProbe::IBLProbe(const CubeMap &env) : cube(env), convoluted(false) {
}

CubeMap IBLProbe::getCubeMap() {
	if(convoluted) {
		return cube;
	}
	if(cube.synchronize(true)) {
		computeConv();
		return cube;
	}
	return CubeMap();
}

uint IBLProbe::getLevelCount() const {
	return 6;
}

float IBLProbe::getRoughnessPower() const {
	return 2.0;
}

float IBLProbe::remapRoughness(float r) const {
	return pow(r, getRoughnessPower());
}

IBLProbe::BufferData IBLProbe::toBufferData() {
	return BufferData{getCubeMap().getBindlessId(), float(1.0 / getRoughnessPower()), int(getLevelCount()), {0}};
}


void IBLProbe::computeConv() {
	RenderableCubeMap conv(cube.getSize(), ImageFormat::RGBA8, true);

	if(!conv.synchronize(true)) {
		fatal("Unable to create cubemap");
	}

	ComputeShaderInstance *cs = getShader();
	cs->setValue("n_Cube", cube);
	for(uint i = 0; i != getLevelCount(); i++) {
		cs->setValue("roughness", remapRoughness(float(i) / (getLevelCount() + 1)));
		cs->setValue("n_Out", conv, TextureAccess::WriteOnly, i);
		cs->dispatch(math::Vec3ui(cube.getSize() / math::Vec2ui(16 * (i + 1)), 1));
	}

	cube = conv;

	convoluted = true;
}

}
}











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

#include "DeferredCommon.h"
#include "GLContext.h"
#include "VertexArrayFactory.h"

namespace n {
namespace graphics {

const VertexArrayObject<> &getSphere() {
	static VertexArrayObject<> *sphere = 0;
	if(!sphere) {
		sphere = new VertexArrayObject<>(GLContext::getContext()->getVertexArrayFactory()(TriangleBuffer<>::getSphere()));
	}
	return *sphere;
}

const VertexArrayObject<> &getBox() {
	static VertexArrayObject<> *box = 0;
	if(!box) {
		box = new VertexArrayObject<>(GLContext::getContext()->getVertexArrayFactory()(TriangleBuffer<>::getCube()));
	}
	return *box;
}

core::String getBRDFs() {
	return "float cook_torrance_D(float NoH, float a2) {"
				"return max(0.0, a2 / (pi * sqr(sqr(NoH) * (a2 - 1.0) + 1.0)));"
			"}"

			"float cook_torrance_G(float NoL, float NoV, float K) {"
				"float G1L = NoL / (NoL * (1.0 - K) + K);"
				"float G1V = NoV / (NoV * (1.0 - K) + K);"
				"return saturate(G1L * G1V);"
			"}"

			"float cook_torrance_F(float LoH, float F0) {"
				"return saturate(F0 + (1.0 - F0) * pow(1.0 - LoH, 5.0));"
			"}"

			"float cook_torrance_div(float NoL, float NoV) {"
				"return max(epsilon, 4.0 * NoL * NoV);"
			"}"

			"float brdf_cook_torrance(vec3 L, vec3 V, vec3 N, vec4 M) {"
				"vec3 H = normalize(L + V);"
				"float NoV = saturate(dot(N, V));"
				"float LoH = saturate(dot(L, H));"
				"float NoH = saturate(dot(N, H));"
				"float NoL = saturate(dot(N, L));"
				"float roughness = M.x + epsilon;"
				"float a = sqr(roughness);"
				"float a2 = sqr(a);"
				"float K = sqr(roughness + 1) / 8.0;"

				"float D = cook_torrance_D(NoH, a2);"

				"float G = cook_torrance_G(NoL, NoV, K);"

				"float F = cook_torrance_F(LoH, M.z);"

				"return F * G * D / cook_torrance_div(NoL, NoV);"
			"}"


			"float brdf_lambert(vec3 L, vec3 V, vec3 N, vec4 M) {"
				"return 1.0 / pi;"
			"}";

}

}
}

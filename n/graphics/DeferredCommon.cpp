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
	return
			// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
			"float D_GGX(float NoH, float a2) {"
				"float d = (NoH * a2 - NoH) * NoH + 1.0;"
				"return a2 / (pi * d * d);"
			"}"

			// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
			"float F_Schlick(float LoH, float F0) {"
				"return F0 + (1.0 - F0) * pow(1.0 - LoH, 5.0);"
			"}"

			"vec3 F_Schlick(float LoH, vec3 F0) {"
				"return F0 + (1.0 - F0) * pow(1.0 - LoH, 5.0);"
			"}"

			"float V_Schlick(float NoL, float NoV, float a) {"
				"float k = a * 0.5;"
				"float GV = NoV * (1.0 - k) + k;"
				"float GL = NoL * (1.0 - k) + k;"
				"return 0.25 / (GL * GV);"
			"}"

			"float brdf_cook_torrance(vec3 L, vec3 V, vec3 N, vec4 M) {"
				"vec3 H = normalize(L + V);"

				"float NoL = saturate(dot(N, L));"
				"float NoV = saturate(dot(N, V));"
				"float LoH = saturate(dot(L, H));"
				"float NoH = saturate(dot(N, H));"

				"float roughness = saturate(M.x + epsilon);"
				"float a = sqr(roughness);"
				"float a2 = sqr(a);"

				"return F_Schlick(LoH, M.z) * D_GGX(NoH, a) * V_Schlick(NoL, NoV, a);"
			"}"

			"float brdf_lambert(vec3 L, vec3 V, vec3 N, vec4 M) {"
				"return 1.0 / pi;"
			"}"

			//http://blog.selfshadow.com/publications/s2013-shading-course/#course_content
			"vec3 brdf_importance_sample(vec2 Xi, float a, vec3 N) {"
				"float phi = 2.0 * pi * Xi.x;"
				"float cosT = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));"
				"float sinT = sqrt(1.0 - cosT * cosT);"
				"vec3 H = vec3(sinT * cos(phi), sinT * sin(phi), cosT);"
				"vec3 U = abs(N.z) > 0.999 ? vec3(1, 0, 0) : vec3(0, 0, 1);"
				"vec3 X = normalize(cross(N, U));"
				"vec3 Y = cross(X, N);"
				"return H.z * N + H.y * Y + H.x * X;"
			"}"

			"vec2 brdf_integrate(float roughness, float NoV) {"
				"vec3 V = vec3(sqrt(1.0 - sqr(NoV)), 0.0, NoV);"
				"vec2 I = vec2(0);"
				"float a = roughness * roughness;"
				"const uint samples = 1024;"
				"for(uint i = 0; i != samples; i++) {"
					"vec2 Xi = hammersley(i, samples);"
					"vec3 H = brdf_importance_sample(Xi, a, V);" // N ?
					"vec3 L = 2.0 * dot(V, H) * H - V;"
					"float NoL = saturate(L.z);"
					"float NoH = saturate(H.z);"
					"float VoH = saturate(dot(V, H));"
					"if(NoL > 0.0) {"
						"float vis = /*D_GGX(NoH, a * a) **/ V_Schlick(NoL, NoV, a);"
						"float Fc = pow(1.0 - VoH, 5.0);"
						"I += vec2(1.0 - Fc, Fc) * vis;"
					"}"
				"}"
				"return I / samples;"
			"}";


}

}
}

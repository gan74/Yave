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
			"float F_Schlick(float VoH, float F0) {"
				"float Fc = pow(1 - VoH, 5.0);"
				"return Fc + (1.0 - Fc) * F0;"
			"}"

			"vec3 F_Schlick(float VoH, vec3 F0) {"
				"float Fc = pow(1 - VoH, 5.0);"
				"return Fc + (1.0 - Fc) * F0;"
			"}"

			"float V_Schlick(float NoL, float NoV, float roughness) {"
				"float k = sqr((roughness + 1.0) * 0.5);"
				"float GV = NoV * (1.0 - k) + k;"
				"float GL = NoL * (1.0 - k) + k;"
				"return 0.25 / (GL * GV + epsilon);"
			"}"

			"float V_Smith(float NoL, float NoV, float a2) {"
				"float GV = NoV + sqrt(NoV * (NoV - NoV * a2) + a2);"
				"float GL = NoL + sqrt(NoL * (NoL - NoL * a2) + a2);"
				"return 1.0 / (GL * GV + epsilon);"
			"}"

			"float V_SmithApprox(float NoL, float NoV, float a) {"
				"float GV = NoL * (NoV * (1.0 - a) + a);"
				"float GL = NoV * (NoL * (1.0 - a) + a);"
				"return 0.5 / (GV + GL + epsilon);"
			"}"

			"vec3 brdf_cook_torrance(vec3 L, vec3 V, vec3 N, float roughness, vec3 F0) {"
				"vec3 H = normalize(L + V);"

				"float NoL = saturate(dot(N, L));"
				"float NoV = max(dot(N, V), epsilon);"
				"float VoH = saturate(dot(V, H));"
				"float NoH = saturate(dot(N, H));"

				//"roughness = saturate(roughness + epsilon);"
				"float a = sqr(roughness);"

				"return "
					"F_Schlick(VoH, F0) * "
					"D_GGX(NoH, a) * "
					"V_Schlick(NoL, NoV, roughness);"
			"}"

			"vec3 brdf_lambert(vec3 L, vec3 V, vec3 N, float roughness, vec3 F0) {"
				"return F0 / pi;"
			"}"

			//http://blog.selfshadow.com/publications/s2013-shading-course/#course_content
			"vec3 brdf_importance_sample(vec2 Xi, float a) {"
				"float phi = 2.0 * pi * Xi.x;"
				"float cosT = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));"
				"float sinT = sqrt(1.0 - cosT * cosT);"
				"return vec3(sinT * cos(phi), sinT * sin(phi), cosT);"
			"}";


}

}
}

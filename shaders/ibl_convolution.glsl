#ifndef IBL_CONVOLUTION_GLSL
#define IBL_CONVOLUTION_GLSL

#include "yave.glsl"


vec3 diffuse_convolution(samplerCube envmap, vec3 normal) {
	vec3 acc = vec3(0.0);
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 right = normalize(cross(up, normal));
	up = cross(normal, right);
	const float sample_delta = 0.05;
	float samples = 0.0;
	for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
		for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
			vec3 tangent_sample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sample_dir = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;

			acc += texture(envmap, sample_dir).rgb * cos(theta) * sin(theta);
			++samples;
		}
	}
	return acc / samples * pi;
}

vec3 diffuse_convolution(sampler2D envmap, vec3 normal) {
	vec3 acc = vec3(0.0);
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 right = normalize(cross(up, normal));
	up = cross(normal, right);
	const float sample_delta = 0.05;
	float samples = 0.0;
	for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
		for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
			vec3 tangent_sample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sample_dir = tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;

			acc += texture(envmap, to_equirec(sample_dir)).rgb * cos(theta) * sin(theta);
			++samples;
		}
	}
	return acc / samples * pi;
}





vec3 specular_convolution(samplerCube envmap, vec3 N, float roughness) {
	const vec3 V = N;
	float total = 0.0;
	vec3 acc = vec3(0.0);
	const uint sample_count = 1024;
	for(uint i = 0; i != sample_count; ++i) {
		vec2 Xi = hammersley(i, sample_count);
		vec3 H  = importance_sample_GGX(Xi, N, roughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);
		float NoL = dot(N, L);
		if(NoL > 0.0) {
			acc += texture(envmap, L).rgb * NoL;
			total += NoL;
		}
	}
	return acc / total;
}

vec3 specular_convolution(sampler2D envmap, vec3 N, float roughness) {
	const vec3 V = N;
	float total = 0.0;
	vec3 acc = vec3(0.0);
	const uint sample_count = 1024;
	for(uint i = 0; i != sample_count; ++i) {
		vec2 Xi = hammersley(i, sample_count);
		vec3 H  = importance_sample_GGX(Xi, N, roughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);
		float NoL = dot(N, L);
		if(NoL > 0.0) {
			acc += texture(envmap, to_equirec(L)).rgb * NoL;
			total += NoL;
		}
	}
	return acc / total;
}

#endif



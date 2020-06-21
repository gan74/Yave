#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "brdf.glsl"

// https://github.com/google/filament/blob/main/shaders/src/shading_model_standard.fs

float attenuation(float distance, float radius) {
	const float x = min(distance, radius);
	return sqr(1.0 - sqr(sqr(x / radius))) / (sqr(x) + 1.0);
}

float attenuation(float distance, float radius, float falloff) {
	return attenuation(distance * falloff, radius * falloff);
}

vec3 approx_F0(float metallic, vec3 albedo) {
	return mix(vec3(0.04), albedo, metallic);
}

vec3 L0(vec3 normal, vec3 light_dir, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	const vec3 half_vec = normalize(light_dir + view_dir);

	const float NoV = max(0.0, dot(normal, view_dir));
	const float NoL = max(0.0, dot(normal, light_dir));
	const float NoH = max(0.0, dot(normal, half_vec));
	const float VoH = max(0.0, dot(view_dir, half_vec));

	const vec3 f0 = approx_F0(metallic, albedo);

	const vec3 kS = brdf_cook_torrance(f0, roughness, NoH, NoV, NoL, VoH);
	const vec3 kD = albedo * brdf_lambert();

	return (kS + kD) * NoL;
}

#endif // LIGHTING_GLSL
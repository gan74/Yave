#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "utils.glsl"


float attenuation(float distance, float radius) {
	const float x = min(distance, radius);
	return sqr(1.0 - sqr(sqr(x / radius))) / (sqr(x) + 1.0);
}

float attenuation(float distance, float radius, float falloff) {
	return attenuation(distance * falloff, radius * falloff);
}

// https://google.github.io/filament/Filament.html
float D_GGX(float NoH, float roughness) {
	const float a2 = sqr(sqr(roughness));
	const float denom = sqr(sqr(NoH) * (a2 - 1.0) + 1.0) * pi;
	return a2 / denom;
}

float G_sub_GGX(float NoV, float k) {
	const float denom = NoV * (1.0 - k) + k;
	return NoV / denom;
}

float G_GGX(float NoV, float NoL, float k) {
	// k for direct lighting = sqr(roughness + 1.0) / 8.0;
	// k for IBL             = sqr(roughness) / 2.0;

	const float ggx_v = G_sub_GGX(NoV, k);
	const float ggx_l = G_sub_GGX(NoL, k);
	return ggx_v * ggx_l;
}

vec3 approx_F0(float metallic, vec3 albedo) {
	return mix(vec3(0.04), albedo, metallic);
}

vec3 F_Schlick(float NoV, vec3 F0) {
	return F0 + (vec3(1.0) - F0) * pow(1.0 - NoV, 5.0);
}

vec3 F_Schlick(float NoV, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NoV, 5.0);
}

float brdf_cook_torrance(float NoH, float NoV, float NoL, float roughness) {
	const float D = D_GGX(NoH, roughness);

	const float k_direct = sqr(roughness + 1.0) / 8.0;
	const float G = G_GGX(NoV, NoL, k_direct);

	const float denom = 4.0 * NoL * NoV + epsilon;

	return D * G / denom;
}

float brdf_lambert() {
	return 1.0 / pi;
}

vec3 L0(vec3 normal, vec3 light_dir, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	// maybe investigate optimisations described here: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
	const vec3 half_vec = normalize(light_dir + view_dir);

	const float NoH = max(0.0, dot(normal, half_vec));
	const float NoV = max(0.0, dot(normal, view_dir));
	const float NoL = max(0.0, dot(normal, light_dir));
	const float HoV = max(0.0, dot(half_vec, view_dir));

	const vec3 F = F_Schlick(HoV, approx_F0(metallic, albedo));

	const vec3 kS = F;
	const vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	const vec3 specular = kS * brdf_cook_torrance(NoH, NoV, NoL, roughness);
	// Won't this account for the albedo twice?
	const vec3 diffuse = kD * albedo * brdf_lambert();

	return (diffuse + specular) * NoL;
}


#endif // LIGHTING_GLSL
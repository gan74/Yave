#ifndef BRDF_GLSL
#define BRDF_GLSL

#include "utils.glsl"

// https://google.github.io/filament/Filament.html
// https://github.com/google/filament/blob/main/shaders/src/brdf.fs
// https://github.com/google/filament/blob/main/shaders/src/shading_model_standard.fs
// Everything here is stolen directly from the filament code base

float D_GGX(float roughness, float NoH) {
	// Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
	const float rev_NoH2 = 1.0 - sqr(NoH);
	const float a2 = sqr(NoH * roughness);
	const float k = roughness / (rev_NoH2 + a2);
	const float d = sqr(k) * (1.0 / pi);
	return saturate(d);
}

float V_Smith_GGX(float roughness, float NoV, float NoL) {
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	const float a2 = sqr(roughness);
	// TODO: lambda_V can be pre-computed for all the lights, it should be moved out of this function
	const float lambda_V = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
	const float lambda_L = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
	const float v = 0.5 / (lambda_V + lambda_L);
	return saturate(v);
}

float V_Smith_GGX_fast(float roughness, float NoV, float NoL) {
	// Hammon 2017, "PBR Diffuse Lighting for GGX+Smith Microsurfaces"
	float v = 0.5 / mix(2.0 * NoL * NoV, NoL + NoV, roughness);
	return saturate(v);
}

float V_Kelemen(float LoH) {
	// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
	return saturate(0.25 / (LoH * LoH));
}

float V_Neubelt(float NoV, float NoL) {
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return saturate(1.0 / (4.0 * (NoL + NoV - NoL * NoV)));
}

vec3 F_Schlick(vec3 f0, float f90, float VoH) {
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}

float F_Schlick(float VoH) {
	return pow(1.0 - VoH, 5.0);
}

vec3 F_Schlick(vec3 f0, float VoH) {
	const float f = pow(1.0 - VoH, 5.0);
	return f + f0 * (1.0 - f);
}

float F_Schlick(float f0, float f90, float VoH) {
	return f0 + (f90 - f0) * pow(1.0 - VoH, 5.0);
}





// -------------------------------------------------------

float distribution(float roughness, float NoH) {
	return D_GGX(roughness, NoH);
}

float visibility(float roughness, float NoV, float NoL) {
	return V_Smith_GGX(roughness, NoV, NoL);
	//return V_Smith_GGX_Fast(roughness, NoV, NoL);
}

vec3 fresnel(vec3 f0, float VoH) {
	// Where does this come from? 
	const float f90 = saturate(dot(f0, vec3(50.0 * 0.33)));
	return F_Schlick(f0, f90, VoH);
}




// -------------------------------------------------------
float brdf_lambert() {
	return 1.0 / pi;
}

vec3 brdf_cook_torrance(vec3 f0, float roughness, float NoH, float NoV, float NoL, float VoH) {
	const float D = distribution(roughness, NoH);
	const float V = visibility(roughness, NoV, NoL);
	const vec3 F = fresnel(f0, VoH);
	
	return (D * V) * F;
}



#endif // BRDF_GLSL
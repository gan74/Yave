#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "brdf.glsl"
#include "gbuffer.glsl"

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

    const vec3 F0 = approx_F0(metallic, albedo);

    const float NDF = D_GGX(NoH, roughness);
    const float G = G_Smith(NoV, NoL, roughness);
    const vec3 F = F_Schlick(VoH, F0);

    const vec3 kS = F;
    const vec3 kD = (1.0 - kS) * (1.0 - metallic);

    const vec3 specular = kS * max(0.0, NDF * G / cook_torrance_denom(NoV, NoL));
    const vec3 diffuse = kD * albedo * (1.0 / pi);

    return (diffuse + specular) * NoL;
}

vec3 L0(vec3 light_dir, vec3 view_dir, GBufferData gbuffer) {
    return L0(gbuffer.normal, light_dir, view_dir, gbuffer.roughness, gbuffer.metallic, gbuffer.albedo);
}

#endif // LIGHTING_GLSL


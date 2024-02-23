#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "utils.glsl"


// Based on nvpro samples's vk_raytrace implementation
// https://github.com/nvpro-samples/vk_raytrace/blob/master/shaders/pbr_gltf.glsl


float attenuation(float distance, float range, float light_radius) {
    const float num = max(0.0, 1.0 - sqr(sqr(distance / range)));
    return sqr(num / max(distance, light_radius));
}

float spot_attenuation(float cos_angle, vec2 scale_offset) {
    const float att = saturate(cos_angle * scale_offset.x + scale_offset.y);
    return sqr(att);
}








vec3 F_Schlick(vec3 f0, vec3 f90, float VdotH) {
    return f0 + (f90 - f0) * pow(saturate(1.0 - VdotH), 5.0);
}

float F_Schlick(float f0, float f90, float VdotH) {
    return f0 + (f90 - f0) * pow(saturate(1.0 - VdotH), 5.0);
}

float V_GGX(float NdotL, float NdotV, float sqr_alpha) {
    const float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - sqr_alpha) + sqr_alpha);
    const float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - sqr_alpha) + sqr_alpha);
    const float GGX = GGXV + GGXL;
    return GGX > 0.0 ? 0.5 / GGX : 0.0;
}

float D_GGX(float NdotH, float sqr_alpha) {
    const float f = (NdotH * NdotH) * (sqr_alpha - 1.0) + 1.0;
    return sqr_alpha / (pi * sqr(f));
}

float G_Schlick_GGX(float NdotV, float alpha) {
    const float k = alpha * 0.5;
    const float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float G_Smith(float NdotV, float NdotL, float alpha) {
    return G_Schlick_GGX(NdotV, alpha) * G_Schlick_GGX(NdotL, alpha);
}




vec3 eval_diffuse(SurfaceInfo info, vec3 f0, vec3 f90, vec3 V, vec3 N, vec3 L, vec3 H) {
    return info.albedo * (inv_pi * (1.0 - info.metallic));
}

vec3 eval_specular(SurfaceInfo info, vec3 f0, vec3 f90, vec3 Vi, vec3 N, vec3 L, vec3 H) {
    const float NdotL = max(dot(N, L), epsilon);
    const float NdotV = max(abs(dot(N, Vi)), epsilon);
    const float NdotH = dot(N, H);
    const float LdotH = dot(L, H);
    const float VdotH = dot(Vi, H);

    const vec3 F = F_Schlick(f0, f90, VdotH);
    const float V = V_GGX(NdotL, NdotV, info.sqr_alpha);
    const float D = D_GGX(NdotH, info.sqr_alpha);

    return F * V * D;
}

vec3 eval_lighting(SurfaceInfo info, vec3 V, vec3 L) {

   const vec3 N = info.normal;
   const float NdotL = dot(N, L);

    if(NdotL < 0.0) {
        return vec3(0.0);
    }

    const vec3 H = normalize(L + V);

    const vec3 diffuse_contrib = eval_diffuse(info, info.f0, info.f90, V, N, L, H);
    const vec3 specular_contrib = eval_specular(info, info.f0, info.f90, V, N, L, H);

    return (diffuse_contrib + specular_contrib) * NdotL;
}


#endif // LIGHTING_GLSL


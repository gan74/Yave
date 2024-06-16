#ifndef LIGHTING_SLANG
#define LIGHTING_SLANG

#include "utils.slang"


// Based on nvpro samples's vk_raytrace implementation
// https://github.com/nvpro-samples/vk_raytrace/blob/master/shaders/pbr_gltf.glsl


float attenuation(float distance, float range, float light_radius) {
    const float num = max(0.0, 1.0 - sqr(sqr(distance / range)));
    return sqr(num / max(distance, light_radius));
}

float spot_attenuation(float cos_angle, float2 scale_offset) {
    const float att = saturate(cos_angle * scale_offset.x + scale_offset.y);
    return sqr(att);
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



float3 eval_diffuse(SurfaceInfo info, float3 f0, float3 f90, float3 V, float3 N, float3 L, float3 H) {
    return info.albedo * (inv_pi * (1.0 - info.metallic));
}

float3 eval_specular(SurfaceInfo info, float3 f0, float3 f90, float3 Vi, float3 N, float3 L, float3 H) {
    const float NdotL = max(dot(N, L), epsilon);
    const float NdotV = max(abs(dot(N, Vi)), epsilon);
    const float NdotH = dot(N, H);
    const float LdotH = dot(L, H);
    const float VdotH = dot(Vi, H);

    const float3 F = F_Schlick(f0, f90, VdotH);
    const float V = V_GGX(NdotL, NdotV, info.alpha2());
    const float D = D_GGX(NdotH, info.alpha2());

    return F * V * D;
}

float3 eval_lighting(SurfaceInfo info, float3 V, float3 L) {

   const float3 N = info.normal;
   const float NdotL = dot(N, L);

    if(NdotL < 0.0) {
        return float3(0.0);
    }

    const float3 H = normalize(L + V);

    const float3 diffuse_contrib = eval_diffuse(info, info.f0(), info.f90(), V, N, L, H);
    const float3 specular_contrib = eval_specular(info, info.f0(), info.f90(), V, N, L, H);

    return (diffuse_contrib + specular_contrib) * NdotL;
}


#endif // LIGHTING_GLSL


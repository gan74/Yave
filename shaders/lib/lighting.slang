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




float g1_ggx(float n_v, float sqr_alpha) {
    const float d = n_v + sqrt(sqr_alpha + (1.0f - sqr_alpha) * n_v * n_v);
    return (2.0 * n_v) / max(d, epsilon);
}

float g_ggx(float n_l, float n_v, float sqr_alpha) {
    return g1_ggx(n_l, sqr_alpha) * g1_ggx(n_v, sqr_alpha);
}

float d_ggx(float n_h, float sqr_alpha) {
    const float f = (n_h * n_h) * (sqr_alpha - 1.0) + 1.0;
    return sqr_alpha / (pi * sqr(f));
}

float3 f_shlick(float l_h, float3 f0) {
    const float k = 1.0 - l_h;
    return f0 + (float3(1.0) - f0) * sqr(sqr(k)) * k;
}




float3 eval_lambert_diffuse(SurfaceInfo info, float3 v, float3 n, float3 l, float3 h) {
    return info.albedo * (inv_pi * (1.0 - info.metallic));
}

float3 eval_ggx_specular(SurfaceInfo info, float3 v, float3 n, float3 l, float3 h) {
    if(info.perceptual_roughness < epsilon) {
        return float3(0.0);
    }

    const float n_l = max(dot(n, l), epsilon);
    const float n_v = max(abs(dot(v, v)), epsilon);
    const float n_h = dot(n, h);
    const float l_h = dot(l, h);

    const float3 f = f_shlick(l_h, info.f0());
    const float g = g_ggx(n_l, n_l, info.alpha2());
    const float d = d_ggx(n_h, info.alpha2());

    return f * (d * g / (4.0 * n_l * n_v));
}

float3 eval_brdf(SurfaceInfo info, float3 v, float3 l) {
   const float3 n = info.normal;
   const float n_l = dot(n, l);

    if(n_l <= 0.0) {
        return float3(0.0);
    }

    const float3 h = normalize(l + v);

    const float3 diffuse_contrib = eval_lambert_diffuse(info, v, n, l, h);
    const float3 specular_contrib = eval_ggx_specular(info, v, n, l, h);

    return (diffuse_contrib + specular_contrib) * n_l;
}


#endif // LIGHTING_GLSL


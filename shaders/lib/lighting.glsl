#ifndef LIGHTING_GLSL
#define LIGHTING_GLSL

#include "brdf.glsl"
#include "gbuffer.glsl"


float attenuation(float distance, float radius) {
    const float x = min(distance, radius);
    return sqr(1.0 - sqr(sqr(x / radius))) / (sqr(x) + 1.0);
}

float attenuation(float distance, float radius, float falloff) {
    return attenuation(distance * falloff, radius * falloff);
}

vec3 L0(vec3 light_dir, vec3 view_dir, SurfaceInfo surface) {
    const vec3 half_vec = normalize(light_dir + view_dir);

    const float NoV = max(0.0, dot(surface.normal, view_dir));
    const float NoL = max(0.0, dot(surface.normal, light_dir));
    const float NoH = max(0.0, dot(surface.normal, half_vec));
    const float VoH = max(0.0, dot(view_dir, half_vec));

    const vec3  F = F_Schlick(VoH, surface.F0);
    const float D = D_GGX(NoH, surface.perceptual_roughness);
    const float V = V_Smith(NoV, NoL, surface.perceptual_roughness);

    const vec3 kS = F;
    const vec3 kD = (1.0 - kS) * (1.0 - surface.metallic);

    const vec3 specular = kS * max(0.0, D * V);
    const vec3 diffuse = kD * Lambert_diffuse_brdf(surface);

    return (diffuse + specular) * NoL;
}

#endif // LIGHTING_GLSL


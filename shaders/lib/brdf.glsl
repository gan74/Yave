#ifndef BRDF_GLSL
#define BRDF_GLSL

#include "utils.glsl"

// https://github.com/JoeyDeVries/LearnOpenGL/tree/master/src/6.pbr/2.2.1.ibl_specular
// https://learnopengl.com/PBR
// https://google.github.io/filament/Filament.html
// https://github.com/google/filament/blob/main/shaders/src/brdf.fs
// https://github.com/google/filament/blob/main/shaders/src/shading_model_standard.fs


float D_GGX(float NoH, float roughness) {
    float a2 = sqr(sqr(roughness));
    const float denom = (sqr(NoH) * (a2 - 1.0) + 1.0);
    return a2 / (pi * sqr(denom));
}

float G_Schlick_GGX(float NoV, float roughness) {
    const float k = sqr(roughness) / 2.0;
    const float denom = NoV * (1.0 - k) + k;
    return NoV / denom;
}

float G_Smith(float NoV, float NoL, float roughness) {
    return G_Schlick_GGX(NoV, roughness) * G_Schlick_GGX(NoL, roughness);
}

vec3 F_Schlick(float cos_theta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cos_theta, 5.0);
}

vec3 F_Schlick(float cos_theta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cos_theta, 5.0);
}

float cook_torrance_denom(float NoV, float NoL) {
    return 4 * NoV * NoL + 0.001;
}






#endif // BRDF_GLSL


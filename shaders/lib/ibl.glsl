#ifndef IBL_GLSL
#define IBL_GLSL

#include "lighting.glsl"
#include "sh.glsl"


float roughness_to_mip(float roughness, uint mip) {
    return roughness * mip;
}

vec3 eval_ibl(samplerCube probe, sampler2D brdf_lut, vec3 V, SurfaceInfo info) {
    const uint probe_mips = textureQueryLevels(probe);

    const vec3 N = info.normal;
    const vec3 R = reflect(-V, N);

    const float NdotV = max(0.0, dot(N, V));

    const vec3 F = F_Schlick(info.f0, info.f90, NdotV);

    const vec3 kS = F;
    const vec3 kD = (1.0 - kS) * (1.0 - info.metallic);

    const vec3 irradiance = textureLod(probe, N, probe_mips - 1).rgb;
    const vec3 diffuse = kD * irradiance * info.albedo;

    const vec2 brdf = texture(brdf_lut, vec2(NdotV, info.perceptual_roughness)).xy;
    const vec3 prefiltered = textureLod(probe, R, roughness_to_mip(info.perceptual_roughness, probe_mips - 1)).rgb;
    const vec3 specular = prefiltered * (kS * brdf.x + brdf.y);

    return diffuse + specular;
}







vec3 importance_sample_GGX(vec2 Xi, vec3 N, float roughness) {
    const float sqr_alpha = sqr(sqr(roughness));

    const float phi = 2.0 * pi * Xi.x;
    const float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (sqr_alpha - 1.0) * Xi.y));
    const float sin_theta = sqrt(1.0 - sqr(cos_theta));

    const vec3 H = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    const vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 T = normalize(cross(up, N));
    const vec3 B = cross(N, T);

    return normalize(T * H.x + B * H.y + N * H.z);
}

vec2 integrate_brdf(float NdotV, float roughness) {
    const vec3 V = vec3(sqrt(1.0 - sqr(NdotV)), 0.0, NdotV);

    float A = 0.0;
    float B = 0.0;

    const vec3 N = vec3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024;
    for(uint i = 0; i != SAMPLE_COUNT; ++i) {
        const vec2 Xi = hammersley(i, SAMPLE_COUNT);
        const vec3 H  = importance_sample_GGX(Xi, N, roughness);
        const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = max(0.0, L.z);
        const float NdotH = max(0.0, H.z);
        const float VdotH = max(0.0, dot(V, H));

        if(NdotL > 0.0) {
            float G = G_Smith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    return vec2(A, B) / SAMPLE_COUNT;
}

vec3 specular_convolution(samplerCube envmap, vec3 N, float roughness) {
    const vec3 V = N;

    float total = 0.0;
    vec3 acc = vec3(0.0);

    const uint sample_count = 1024;
    for(uint i = 0; i != sample_count; ++i) {
        const vec2 Xi = hammersley(i, sample_count);
        const vec3 H  = importance_sample_GGX(Xi, N, roughness);
        const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = max(0.0, dot(N, L));

        if(NdotL > 0.0) {
            acc += texture(envmap, L).rgb * NdotL;
            total += NdotL;
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
        const vec2 Xi = hammersley(i, sample_count);
        const vec3 H  = importance_sample_GGX(Xi, N, roughness);
        const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = max(0.0, dot(N, L));

        if(NdotL > 0.0) {
            acc += texture(envmap, to_equirec(L)).rgb * NdotL;
            total += NdotL;
        }
    }
    return acc / total;
}



vec3 diffuse_convolution(samplerCube envmap, vec3 N) {
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 acc = vec3(0.0);
    float samples = 0.0;

    const float sample_delta = 0.05;
    for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
        for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
            const vec3 T = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            const vec3 sample_dir = T.x * right + T.y * up + T.z * N;

            acc += texture(envmap, sample_dir).rgb * cos(theta) * sin(theta);
            ++samples;
        }
    }
    return (acc / samples) * pi;
}

vec3 diffuse_convolution(sampler2D envmap, vec3 N) {
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    const vec3 right = normalize(cross(up, N));
    up = cross(N, right);

    vec3 acc = vec3(0.0);
    float samples = 0.0;

    const float sample_delta = 0.05;
    for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
        for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
            const vec3 T = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            const vec3 sample_dir = T.x * right + T.y * up + T.z * N;

            acc += texture(envmap, to_equirec(sample_dir)).rgb * cos(theta) * sin(theta);
            ++samples;
        }
    }
    return (acc / samples) * pi;
}


#endif // IBL_GLSL


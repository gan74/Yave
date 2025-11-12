#ifndef BRDF_SLANG
#define BRDF_SLANG

#include "utils.slang"


// Based on nvpro samples's vk_raytrace implementation
// https://github.com/nvpro-samples/vk_raytrace/blob/master/shaders/pbr_gltf.glsl






float g1_ggx(float n_v, float sqr_alpha) {
    const float d = n_v + sqrt(sqr_alpha + (1.0f - sqr_alpha) * n_v * n_v);
    return (2.0 * n_v) / max(d, constants.epsilon);
}

float g_ggx(float n_l, float n_v, float sqr_alpha) {
    return g1_ggx(n_l, sqr_alpha) * g1_ggx(n_v, sqr_alpha);
}

float d_ggx(float n_h, float sqr_alpha) {
    const float f = (n_h * n_h) * (sqr_alpha - 1.0) + 1.0;
    return sqr_alpha / (constants.pi * sqr(f));
}

float3 f_shlick(float l_h, float3 f0) {
    const float k = 1.0 - l_h;
    return f0 + (float3(1.0) - f0) * sqr(sqr(k)) * k;
}

float g_schlick_ggx(float n_l, float roughness) {
    const float k = sqr(roughness) * 0.5;
    const float denom = n_l * (1.0 - k) + k;
    return n_l / denom;
}

float g_smith(float n_v, float n_l, float roughness) {
    return g_schlick_ggx(max(0.0, n_v), roughness) * g_schlick_ggx(max(0.0, n_l), roughness);
}

float3 f_schlick_roughness(float n_v, float3 f0, float roughness) {
    const float3 f90 = max(float3(1.0 - roughness), f0);
    return f0 + (f90 - f0) * pow(saturate(1.0 - n_v), 5.0);
}






float3 diffuse_convolution<T: Envmap>(T envmap, float3 n) {
    float3 up = abs(n.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 right = normalize(cross(up, n));
    up = cross(n, right);

    float3 acc = float3(0.0);
    float samples = 0.0;

    const float sample_delta = 0.05;
    for(float phi = 0.0; phi < 2.0 * constants.pi; phi += sample_delta) {
        for(float theta = 0.0; theta < 0.5 * constants.pi; theta += sample_delta) {
            const float3 t = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            const float3 sample_dir = t.x * right + t.y * up + t.z * n;

            acc += envmap.SampleCube(sample_dir).rgb * cos(theta) * sin(theta);
            ++samples;
        }
    }
    return (acc / samples) * constants.pi;
}

float3 specular_convolution<T: Envmap>(T envmap, float3 n, float roughness) {
    const float3 v = n;

    float total = 0.0;
    float3 acc = float3(0.0);

    const uint sample_count = 1024;
    for(uint i = 0; i != sample_count; ++i) {
        const float2 xi = hammersley(i, sample_count);
        const float3 h  = importance_sample_ggx(xi, n, roughness);
        const float3 l  = normalize(2.0 * dot(v, h) * h - v);

        const float n_l = max(0.0, dot(n, l));

        if(n_l > 0.0) {
            acc += envmap.SampleCube(l).rgb * n_l;
            total += n_l;
        }
    }
    return acc / total;
}

float3 importance_sample_ggx(float2 xi, float3 n, float roughness) {
    const float alpha = sqr(roughness);
    const float sqr_alpha = sqr(alpha);

    const float phi = 2.0 * constants.pi * xi.x;
    const float cos_theta = sqrt((1.0 - xi.y) / (1.0 + (sqr_alpha - 1.0) * xi.y));
    const float sin_theta = sqrt(1.0 - sqr(cos_theta));

    const float3 h = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    const float3 up = abs(n.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 t = normalize(cross(up, n));
    const float3 b = cross(n, t);

    return normalize(t * h.x + b * h.y + n * h.z);
}

float2 integrate_brdf(float n_v, float roughness) {
    const float3 v = float3(sqrt(1.0 - sqr(n_v)), 0.0, n_v);

    float a = 0.0;
    float b = 0.0;

    const uint sample_count = 1024;
    const float3 n = float3(0.0, 0.0, 1.0);

    for(uint i = 0; i != sample_count; ++i) {
        const float2 xi = hammersley(i, sample_count);
        const float3 h  = importance_sample_ggx(xi, n, roughness);
        const float3 l  = normalize(2.0 * dot(v, h) * h - v);

        const float n_l = max(0.0, l.z);
        const float n_h = max(0.0, h.z);
        const float v_h = max(0.0, dot(v, h));

        if(n_l > 0.0) {
            const float g = g_smith(n_v, n_l, roughness);
            const float g_vis = (g * v_h) / (n_h * n_v);
            const float f = pow(1.0 - v_h, 5.0);

            a += (1.0 - f) * g_vis;
            b += f * g_vis;
        }
    }
    return float2(a, b) / sample_count;
}

float3 eval_ibl(SamplerCube probe, Sampler2D<float2> brdf_lut, float3 v, SurfaceInfo info) {
    const uint probe_mips = image_mips(probe);

    const float3 n = info.normal;
    const float3 r = reflect(-v, n);

    const float n_v = max(0.0, dot(n, v));

    const float3 f = f_schlick_roughness(n_v, info.f0(), info.perceptual_roughness);

    const float3 k_s = f;
    const float3 k_d = (1.0 - k_s) * (1.0 - info.metallic);

    const float3 irradiance = probe.SampleLevel(n, probe_mips - 1).rgb;
    const float3 diffuse = k_d * irradiance * info.albedo;

    const float2 brdf = brdf_lut.Sample(float2(n_v, info.perceptual_roughness)).xy;
    const float3 prefiltered = probe.SampleLevel(r, roughness_to_mip(info.perceptual_roughness, probe_mips - 1)).rgb;
    const float3 specular = prefiltered * (k_s * brdf.x + brdf.y);

    return diffuse + specular;
}


struct SpecularContrib {
    float3 ks;
    float contrib;
};

SpecularContrib eval_ggx_specular(SurfaceInfo info, float3 v, float3 n, float3 l, float3 h) {
    const float l_h = dot(l, h);
    const float3 f = f_shlick(l_h, info.f0());

    if(info.perceptual_roughness < constants.epsilon) {
        return SpecularContrib(f, 0.0);
    }

    const float n_l = max(dot(n, l), constants.epsilon);
    const float n_v = max(abs(dot(v, v)), constants.epsilon);
    const float n_h = dot(n, h);

    const float g = g_ggx(n_l, n_l, info.alpha2());
    const float d = d_ggx(n_h, info.alpha2());

    return SpecularContrib(f, d * g / (4.0 * n_l * n_v));
}



float3 eval_lambert_diffuse(SurfaceInfo info, float3 v, float3 n, float3 l, float3 h) {
    return info.albedo * (constants.inv_pi * (1.0 - info.metallic));
}




float3 eval_brdf(SurfaceInfo info, float3 v, float3 l) {
   const float3 n = info.normal;
   const float n_l = dot(n, l);

    if(n_l <= 0.0) {
        return float3(0.0);
    }

    const float3 h = normalize(l + v);

    const SpecularContrib specular = eval_ggx_specular(info, v, n, l, h);
    const float3 diffuse_contrib = eval_lambert_diffuse(info, v, n, l, h);

    const float3 kd = saturate(1.0 - specular.ks);

    return (diffuse_contrib * kd + specular.contrib * specular.ks) * n_l;
}


#endif // BRDF_SLANG


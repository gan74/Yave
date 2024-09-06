#ifndef UTILS_SLANG
#define UTILS_SLANG

#include "semantics.slang"
#include "structs.slang"


static const float pi = 3.1415926535897932384626433832795;
static const float inv_pi = 1.0 / pi;
static const float euler = 2.7182818284590452353602874713526;
static const float epsilon = 0.0000001;

static const uint max_uint = uint(0xFFFFFFFF);
static const float max_float = 3.402823e+38;

static const uint max_bones = 256;
static const uint max_tile_lights = 256;

static const float lum_histogram_offset = 8.0;
static const float lum_histogram_mul = 8.0;
static const uint lum_histogram_size = 256;

static const float golden = 1.618033988749;



struct StdVertexStageIn {
    float3 position;
    uint2 packed_normal_tangent_sign;
    float2 uv;
};


struct SurfaceInfo {
    float3 albedo;
    float perceptual_roughness;

    float metallic;
    float3 normal;

    float alpha() {
        return sqr(perceptual_roughness);
    }

    float alpha2() {
        return sqr(alpha());
    }

    float3 f0() {
        return approx_F0(metallic, albedo);
    }

    float3 f90() {
        return F90_from_F0(f0());
    }
};


struct RawGBuffer {
    float4 rt0;
    float4 rt1;

    __init() {
        rt0 = rt1 = 0.0;
    }

    __init(float4 r0, float4 r1) {
        rt0 = r0;
        rt1 = r1;
    }
}





T sqr<T: IArithmetic>(T t) {
    return t * t;
}






void unpack_gbuffer_color(float4 buff, out float3 color, out float metallic) {
    color = buff.rgb;
    metallic = buff.a;
}

void unpack_gbuffer_normal(float4 buff, out float3 normal, out float roughness) {
    normal = octahedron_decode(buff.xy);
    roughness = max(0.05, buff.z);
}

float4 pack_gbuffer_color(float3 color, float metallic) {
    return float4(color, metallic);
}

float4 pack_gbuffer_normal(float3 normal, float roughness) {
    return float4(octahedron_encode(normal), roughness, 0.0);
}


RawGBuffer encode_gbuffer(SurfaceInfo info) {
    RawGBuffer raw;
    raw.rt0 = pack_gbuffer_color(info.albedo, info.metallic);
    raw.rt1 = pack_gbuffer_normal(info.normal, info.perceptual_roughness);
    return raw;
}

SurfaceInfo decode_gbuffer(RawGBuffer raw) {
    SurfaceInfo surface;
    unpack_gbuffer_color(raw.rt0, surface.albedo, surface.metallic);
    unpack_gbuffer_normal(raw.rt1, surface.normal, surface.perceptual_roughness);
    return surface;
}








float luminance(float3 rgb) {
    return dot(rgb, float3(0.2126, 0.7152, 0.0722));
}

float avg_to_EV100(float avg) {
    return log2(avg * 100.0 / 12.5);
}

// We divide by 1.2 to avoid crushing spec too much
float EV100_to_exposure(float EV100) {
    float max_lum = 1.2 * pow(2.0, EV100);
    return 1.0 / max_lum;
}

float3 expose_RGB(float3 hdr, float exposure) {
    return hdr * exposure;
}

float uncharted2(float x) {
    const float a = 0.22;
    const float b = 0.30;
    const float c = 0.10;
    const float d = 0.20;
    const float e = 0.01;
    const float f = 0.30;
    return ((x * (a * x + c * b) + d * e) / (x * (a * x + b) + d * f)) - e / f;
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
float ACES(float x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return ((x * (a * x + b)) / (x * (c * x + d) + e));
}

float reinhard(float hdr) {
    return hdr / (hdr + 1.0);
}

float3 uncharted2(float3 x) {
    return float3(uncharted2(x.x), uncharted2(x.y), uncharted2(x.z));
}

float3 ACES(float3 x) {
    return float3(ACES(x.x), ACES(x.y), ACES(x.z));
}

float3 reinhard(float3 x) {
    return float3(reinhard(x.x), reinhard(x.y), reinhard(x.z));
}

uint lum_to_histogram_bin(float lum) {
    const float log_lum = log2(lum) + lum_histogram_offset;
    if(log_lum <= 0.0) {
        return 0;
    }
    return min(lum_histogram_size - 1, uint(log_lum * lum_histogram_mul));
}

float histogram_bin_to_lum(uint bin) {
    const float log_lum = bin / lum_histogram_mul - lum_histogram_offset;
    return exp2(log_lum);
}






uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >>  6u);
    x += (x <<  3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

uint hash(uint2 x) {
    const uint2 q = 1103515245u * ((x >> 1u) ^ x.yx);
    return 1103515245u * (q.x ^ (q.y >> 3u));
}






float3 uv_debug_color(float2 p) {
    float3 p3 = fract(p.xyx * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yzz) * p3.zyx);
}

float3 heat_spectrum(float x) {
    x *= 4.0;
    float3 color = lerp(float3(0, 0, 1), float3(0, 1, 0), saturate(x));
    color = lerp(color, float3(1, 1, 0), saturate(x - 1.0));
    color = lerp(color, float3(1, 0, 0), saturate(x - 2.0));
    color = lerp(color, float3(1, 1, 1), saturate(x - 3.0));
    return color / max(color.x, max(color.y, color.z));
}





// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
float2 octahedron_wrap(float2 v) {
    return (float2(1.0) - abs(v.yx)) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

float2 octahedron_encode(float3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : octahedron_wrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

float3 octahedron_decode(float2 f) {
    f = f * 2.0 - 1.0;
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3(f.xy, 1.0 - abs(f.x) - abs(f.y));
    const float t = saturate(-n.z);
    n.xy += float2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
    return normalize(n);
}


float3 unpack_normal_map(float2 normal) {
    normal = normal * 2.0 - 1.0;
    return float3(normal, 1.0 - sqrt(dot(normal, normal)));
}


float4 unpack_2_10_10_10(uint packed) {
    return float4(
        (float3(
            (packed >> 20) & 0x03FF,
            (packed >> 10) & 0x03FF,
            (packed >>  0) & 0x03FF
        ) / float(0x03FF)) * 2.0 - 1.0,
        (packed >> 30 == 0) ? 1.0 : -1.0
    );
}

float4 unpack_color(uint packed) {
    return float4(
        (packed >> 0) & 0xFF,
        (packed >> 8) & 0xFF,
        (packed >> 16) & 0xFF,
        (packed >> 24) & 0xFF
    ) / 255.0;
}

float2 hammersley(uint i, uint N) {
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    const float radical_inverse = float(bits) * 2.3283064365386963e-10;
    return float2(float(i) / float(N), radical_inverse);
}

// https://observablehq.com/@meetamit/fibonacci-lattices
float2 fibonacci_square(uint i, uint count) {
    return float2(
        fract(i / golden),
        i / (count - 0.9)
    );
}

float3 fibonacci_sphere(uint i, uint count) {
    const float2 sq = fibonacci_square(i, count);
    const float2 theta_phi = float2(2.0 * pi * sq.x, acos(1.0 - 2.0 * sq.y));
    return float3(
        float2(cos(theta_phi.x), sin(theta_phi.x)) * sin(theta_phi.y),
        cos(theta_phi.y)
    );
}

bool is_depth_far(float z) {
    return z <= 0.0; // reversed Z
}

bool is_depth_near(float z) {
    return z >= 1.0; // reversed Z
}

float3 unproject_ndc(float3 ndc, float4x4 inv_matrix) {
    const float4 p = mul(inv_matrix, float4(ndc, 1.0));
    return p.xyz / p.w;
}

float3 unproject(float2 uv, float depth, float4x4 inv_matrix) {
    const float3 ndc = float3(uv * 2.0 - 1.0, depth);
    return unproject_ndc(ndc, inv_matrix);
}

float3 project(float3 pos, float4x4 proj_matrix) {
    const float4 p = mul(proj_matrix, float4(pos, 1.0));
    const float3 p3 = p.xyz / p.w;
    return float3(p3.xy * 0.5 + float2(0.5), p3.z);
}

float3 build_orthogonal(float3 v) {
    const float3 t = abs(v.x) > 0.9 ? float3(0, 1, 0) : float3(1, 0, 0);
    return normalize(cross(v, t));
}




struct CubeMap : IEnvmap {
    SamplerCube<float4> sampler;

    float4 SampleCube(float3 dir, float level = 0.0) {
        return sampler.SampleLevel(dir, level);
    }
};

struct EquirecMap : IEnvmap {
    Sampler2D sampler;

    float4 SampleCube(float3 dir, float level = 0.0) {
        return sampler.SampleLevel(to_equirec(dir), level);
    }
};


interface IEnvmap {
    float4 SampleCube(float3 dir, float level = 0.0);
}





float2 to_equirec(float3 v) {
    return -float2(atan2(-v.y, v.x), asin(v.z)) * float2(0.1591, 0.3183) + 0.5;
}


float3 importance_sample_GGX(float2 Xi, float3 N, float roughness) {
    const float sqr_alpha = sqr(sqr(roughness));

    const float phi = 2.0 * pi * Xi.x;
    const float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (sqr_alpha - 1.0) * Xi.y));
    const float sin_theta = sqrt(1.0 - sqr(cos_theta));

    const float3 H = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    const float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 T = normalize(cross(up, N));
    const float3 B = cross(N, T);

    return normalize(T * H.x + B * H.y + N * H.z);
}

float3 diffuse_convolution<T: IEnvmap>(T envmap, float3 N) {
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 right = normalize(cross(up, N));
    up = cross(N, right);

    float3 acc = float3(0.0);
    float samples = 0.0;

    const float sample_delta = 0.05;
    for(float phi = 0.0; phi < 2.0 * pi; phi += sample_delta) {
        for(float theta = 0.0; theta < 0.5 * pi; theta += sample_delta) {
            const float3 T = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            const float3 sample_dir = T.x * right + T.y * up + T.z * N;

            acc += envmap.SampleCube(sample_dir).rgb * cos(theta) * sin(theta);
            ++samples;
        }
    }
    return (acc / samples) * pi;
}

float3 specular_convolution<T: IEnvmap>(T envmap, float3 N, float roughness) {
    const float3 V = N;

    float total = 0.0;
    float3 acc = float3(0.0);

    const uint sample_count = 1024;
    for(uint i = 0; i != sample_count; ++i) {
        const float2 Xi = hammersley(i, sample_count);
        const float3 H  = importance_sample_GGX(Xi, N, roughness);
        const float3 L  = normalize(2.0 * dot(V, H) * H - V);

        const float NdotL = max(0.0, dot(N, L));

        if(NdotL > 0.0) {
            acc += envmap.SampleCube(L).rgb * NdotL;
            total += NdotL;
        }
    }
    return acc / total;
}

float2 integrate_brdf(float NdotV, float roughness) {
    const float3 V = float3(sqrt(1.0 - sqr(NdotV)), 0.0, NdotV);

    float A = 0.0;
    float B = 0.0;

    const float3 N = float3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024;
    for(uint i = 0; i != SAMPLE_COUNT; ++i) {
        const float2 Xi = hammersley(i, SAMPLE_COUNT);
        const float3 H  = importance_sample_GGX(Xi, N, roughness);
        const float3 L  = normalize(2.0 * dot(V, H) * H - V);

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
    return float2(A, B) / SAMPLE_COUNT;
}

float roughness_to_mip(float roughness, uint mip) {
    return roughness * mip;
}

float3 eval_ibl(SamplerCube probe, Sampler2D<float2> brdf_lut, float3 V, SurfaceInfo info) {
    const uint probe_mips = image_mips(probe);

    const float3 N = info.normal;
    const float3 R = reflect(-V, N);

    const float NdotV = max(0.0, dot(N, V));

    const float3 F = F_Schlick(info.f0(), info.f90(), NdotV);

    const float3 kS = F;
    const float3 kD = (1.0 - kS) * (1.0 - info.metallic);

    const float3 irradiance = probe.SampleLevel(N, probe_mips - 1).rgb;
    const float3 diffuse = kD * irradiance * info.albedo;

    const float2 brdf = brdf_lut.Sample(float2(NdotV, info.perceptual_roughness)).xy;
    const float3 prefiltered = probe.SampleLevel(R, roughness_to_mip(info.perceptual_roughness, probe_mips - 1)).rgb;
    const float3 specular = prefiltered * (kS * brdf.x + brdf.y);

    return diffuse + specular;
}






float2 atlas_uv(ShadowMapInfo info, float2 uv) {
    return info.uv_offset + uv * info.uv_mul;
}

float sample_shadow(Sampler2DShadow shadow_map, float3 proj, ShadowMapInfo info, float bias) {
    const float2 uv = atlas_uv(info, proj.xy);
    return shadow_map.SampleCmpLevelZero(uv, proj.z + bias);
}

float compute_shadow_hard(Sampler2DShadow shadow_map, ShadowMapInfo info, float3 world_pos, float bias) {
    const float3 proj = project(world_pos, info.view_proj);

    if(any(saturate(proj) != proj)) {
        return 1.0;
    }

    return sample_shadow(shadow_map, proj, info, bias);
}

float compute_shadow_pcf(Sampler2DShadow shadow_map, ShadowMapInfo info, float3 world_pos, float bias) {
    const float3 proj = project(world_pos, info.view_proj);

    if(any(saturate(proj) != proj)) {
        return 1.0;
    }

    const float2 offset = float2(0.5);
    const float2 tx = proj.xy * info.size + offset;
    const float2 base = (floor(tx) - offset) * info.texel_size;
    const float2 st = fract(tx);

#if 0
    const float2 uw = float2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    const float2 vw = float2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    const float2 u = float2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0) * info.texel_size;
    const float2 v = float2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0) * info.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, float3(base + float2(u.x, v.x), proj.z), info, bias);
    sum += uw.y * vw.x * sample_shadow(shadow_map, float3(base + float2(u.y, v.x), proj.z), info, bias);
    sum += uw.x * vw.y * sample_shadow(shadow_map, float3(base + float2(u.x, v.y), proj.z), info, bias);
    sum += uw.y * vw.y * sample_shadow(shadow_map, float3(base + float2(u.y, v.y), proj.z), info, bias);
    return sum / 16.0;
#else
    const float3 uw = float3(4.0 - 3.0 * st.x, 7.0, 1.0 + 3.0 * st.x);
    const float3 vw = float3(4.0 - 3.0 * st.y, 7.0, 1.0 + 3.0 * st.y);

    const float3 u = float3((3.0 - 2.0 * st.x) / uw.x - 2.0, (3.0 + st.x) / uw.y, st.x / uw.z + 2.0) * info.texel_size;
    const float3 v = float3((3.0 - 2.0 * st.y) / vw.x - 2.0, (3.0 + st.y) / vw.y, st.y / vw.z + 2.0) * info.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, float3(base + float2(u.x, v.x), proj.z), info, bias);
    sum += uw.y * vw.x * sample_shadow(shadow_map, float3(base + float2(u.y, v.x), proj.z), info, bias);
    sum += uw.z * vw.x * sample_shadow(shadow_map, float3(base + float2(u.z, v.x), proj.z), info, bias);
    sum += uw.x * vw.y * sample_shadow(shadow_map, float3(base + float2(u.x, v.y), proj.z), info, bias);
    sum += uw.y * vw.y * sample_shadow(shadow_map, float3(base + float2(u.y, v.y), proj.z), info, bias);
    sum += uw.z * vw.y * sample_shadow(shadow_map, float3(base + float2(u.z, v.y), proj.z), info, bias);
    sum += uw.x * vw.z * sample_shadow(shadow_map, float3(base + float2(u.x, v.z), proj.z), info, bias);
    sum += uw.y * vw.z * sample_shadow(shadow_map, float3(base + float2(u.y, v.z), proj.z), info, bias);
    sum += uw.z * vw.z * sample_shadow(shadow_map, float3(base + float2(u.z, v.z), proj.z), info, bias);
    return sum / 144.0;
#endif
}


float compute_shadow_hard(Sampler2DShadow shadow_map, ShadowMapInfo info, float3 world_pos) {
    return compute_shadow_hard(shadow_map, info, world_pos, 0.0);
}

float compute_shadow_pcf(Sampler2DShadow shadow_map, ShadowMapInfo info, float3 world_pos) {
    return compute_shadow_pcf(shadow_map, info, world_pos, 0.0);
}









struct AreaLightInfo {
    float3 light_dir;
    float orig_light_dist;
    float light_dist;
    float sqr_alpha;
};

// https://alextardif.com/arealights.html
AreaLightInfo karis_area_light(SurfaceInfo surface, float3 light_pos, float source_radius, float3 world_pos, float3 view_dir) {
    const float3 refl = reflect(-view_dir, surface.normal);
    const float3 to_light = light_pos - world_pos;
    const float3 center = (dot(to_light, refl) * refl) - to_light;
    const float3 closest_point = to_light + center * saturate(source_radius / length(center));

    AreaLightInfo info;
    info.light_dir = normalize(closest_point);
    info.orig_light_dist = length(to_light);
    info.light_dist = length(closest_point);
    info.sqr_alpha = saturate(source_radius / (info.light_dist * 2.0) + surface.alpha()) * surface.alpha();
    return info;
}

AreaLightInfo karis_area_light(SurfaceInfo surface, PointLight light, float3 world_pos, float3 view_dir) {
    return karis_area_light(surface, light.position, light.min_radius, world_pos, view_dir);
}

AreaLightInfo karis_area_light(SurfaceInfo surface, SpotLight light, float3 world_pos, float3 view_dir) {
    return karis_area_light(surface, light.position, light.min_radius, world_pos, view_dir);
}

SurfaceInfo alpha_corrected_surface(SurfaceInfo surface, AreaLightInfo area) {
    surface.perceptual_roughness = pow(area.sqr_alpha, 1.0 / 4.0);
    return surface;
}





T F_Schlick<T: IFloat>(T f0, T f90, float VdotH) {
    return f0 + (f90 - f0) * T(pow(saturate(1.0 - VdotH), 5.0));
}

float G_Schlick_GGX(float NdotV, float alpha) {
    const float k = alpha * 0.5;
    const float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float G_Smith(float NdotV, float NdotL, float alpha) {
    return G_Schlick_GGX(NdotV, alpha) * G_Schlick_GGX(NdotL, alpha);
}

float3 approx_F0(float metallic, float3 albedo) {
    return lerp(float3(0.04), albedo, metallic);
}

float3 F90_from_F0(float3 f0) {
    const float reflectance = max(max(f0.r, f0.g), f0.b);
    return saturate(reflectance * 50.0);
}





#endif

#ifndef UTILS_SLANG
#define UTILS_SLANG


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







uint2 image_size<T>(Sampler2D<T> smp) {
    uint2 size;
    smp.GetDimensions(size.x, size.y);
    return size;
}

uint2 image_size<T>(RWTexture2D<T> smp) {
    uint2 size;
    smp.GetDimensions(size.x, size.y);
    return size;
}

uint3 image_size<T>(RWTexture2DArray<T> smp) {
    uint3 size;
    smp.GetDimensions(size.x, size.y, size.z);
    return size;
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

bool is_depth_OOB(float z) {
    return z <= 0.0; // reversed Z
}



float3 unproject_ndc(float3 ndc, float4x4 inv_matrix) {
    const float4 p = mul(inv_matrix, float4(ndc, 1.0));
    return p.xyz / p.w;
}

float3 unproject(float2 uv, float depth, float4x4 inv_matrix) {
    const float3 ndc = float3(uv * 2.0 - 1.0, depth);
    return unproject_ndc(ndc, inv_matrix);
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






float G_Schlick_GGX(float NdotV, float alpha) {
    const float k = alpha * 0.5;
    const float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float G_Smith(float NdotV, float NdotL, float alpha) {
    return G_Schlick_GGX(NdotV, alpha) * G_Schlick_GGX(NdotL, alpha);
}



#endif

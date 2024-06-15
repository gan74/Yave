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






struct SurfaceInfo {
    float3 albedo;
    float perceptual_roughness;

    float metallic;
    float3 normal;
};


struct RawGBuffer {
    float4 rt0;
    float4 rt1;
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

#endif

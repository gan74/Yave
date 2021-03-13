// Based on MiniEngine's implementation
// see: https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/AoBlurAndUpsampleCS.hlsli

#include "lib/utils.glsl"

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform sampler2D in_lo_ao;
layout(set = 0, binding = 1) uniform sampler2D in_hi_ao;
layout(set = 0, binding = 2) uniform sampler2D in_lo_depth;
layout(set = 0, binding = 3) uniform sampler2D in_hi_depth;

layout(set = 0, binding =  4) uniform UpsampleData_Inline {
    float step_size;
    float noise_filter_weight;
    float blur_tolerance;
    float upsample_tolerance;
};

layout(r8, set = 0, binding = 5) uniform writeonly image2D out_ao;

#ifdef COMBINE_HIGH
#define in_ao       in_lo_ao
#endif

#ifdef COMBINE_LOW
#define in_ao       in_hi_ao
#endif

#ifndef in_ao
#define in_ao       in_hi_ao
#endif

shared float depth_samples[256];
shared float ao_samples0[256];
shared float ao_samples1[256];

void prefetch(uint index, vec2 uv) {
    vec4 ao0 = textureGather(in_ao, uv);
    ao_samples0[index]         = ao0.w;
    ao_samples0[index + 1]     = ao0.z;
    ao_samples0[index + 16]    = ao0.x;
    ao_samples0[index + 17]    = ao0.y;

#ifdef COMBINE_LOW
    const vec4 ao1 = textureGather(in_lo_ao, uv);
    ao0 = min(ao0, ao1);
#endif

    const vec4 ids = 1.0 / textureGather(in_lo_depth, uv);
    depth_samples[index]       = ids.w;
    depth_samples[index + 1]   = ids.z;
    depth_samples[index + 16]  = ids.x;
    depth_samples[index + 17]  = ids.y;
}

float smart_blur(float a, float b, float c, float d, float e, bool left, bool middle, bool right) {
    b = left || middle ? b : c;
    a = left ? a : b;
    d = right || middle ? d : c;
    e = right ? e : d;
    return ((a + e) / 2.0 + b + c + d) / 4.0;
}

bool compare_deltas(float d1, float d2, float l1, float l2) {
    float temp = d1 * d2 + step_size;
    return temp * temp > l1 * l2 * blur_tolerance;
}

void horizontal_blur(uint left_index) {
    const float a0 = ao_samples0[left_index];
    const float a1 = ao_samples0[left_index + 1];
    const float a2 = ao_samples0[left_index + 2];
    const float a3 = ao_samples0[left_index + 3];
    const float a4 = ao_samples0[left_index + 4];
    const float a5 = ao_samples0[left_index + 5];
    const float a6 = ao_samples0[left_index + 6];

    const float d0 = depth_samples[left_index];
    const float d1 = depth_samples[left_index + 1];
    const float d2 = depth_samples[left_index + 2];
    const float d3 = depth_samples[left_index + 3];
    const float d4 = depth_samples[left_index + 4];
    const float d5 = depth_samples[left_index + 5];
    const float d6 = depth_samples[left_index + 6];

    const float d01 = d1 - d0;
    const float d12 = d2 - d1;
    const float d23 = d3 - d2;
    const float d34 = d4 - d3;
    const float d45 = d5 - d4;
    const float d56 = d6 - d5;

    const float l01 = d01 * d01 + step_size;
    const float l12 = d12 * d12 + step_size;
    const float l23 = d23 * d23 + step_size;
    const float l34 = d34 * d34 + step_size;
    const float l45 = d45 * d45 + step_size;
    const float l56 = d56 * d56 + step_size;

    const bool c02 = compare_deltas(d01, d12, l01, l12);
    const bool c13 = compare_deltas(d12, d23, l12, l23);
    const bool c24 = compare_deltas(d23, d34, l23, l34);
    const bool c35 = compare_deltas(d34, d45, l34, l45);
    const bool c46 = compare_deltas(d45, d56, l45, l56);

    ao_samples1[left_index + 0] = smart_blur(a0, a1, a2, a3, a4, c02, c13, c24);
    ao_samples1[left_index + 1] = smart_blur(a1, a2, a3, a4, a5, c13, c24, c35);
    ao_samples1[left_index + 2] = smart_blur(a2, a3, a4, a5, a6, c24, c35, c46);
}

void vertical_blur(uint top_index) {
    const float a0 = ao_samples1[top_index];
    const float a1 = ao_samples1[top_index + 16];
    const float a2 = ao_samples1[top_index + 32];
    const float a3 = ao_samples1[top_index + 48];
    const float a4 = ao_samples1[top_index + 64];
    const float a5 = ao_samples1[top_index + 80];

    const float d0 = depth_samples[top_index + 2];
    const float d1 = depth_samples[top_index + 18];
    const float d2 = depth_samples[top_index + 34];
    const float d3 = depth_samples[top_index + 50];
    const float d4 = depth_samples[top_index + 66];
    const float d5 = depth_samples[top_index + 82];

    const float d01 = d1 - d0;
    const float d12 = d2 - d1;
    const float d23 = d3 - d2;
    const float d34 = d4 - d3;
    const float d45 = d5 - d4;

    const float l01 = d01 * d01 + step_size;
    const float l12 = d12 * d12 + step_size;
    const float l23 = d23 * d23 + step_size;
    const float l34 = d34 * d34 + step_size;
    const float l45 = d45 * d45 + step_size;

    const bool c02 = compare_deltas(d01, d12, l01, l12);
    const bool c13 = compare_deltas(d12, d23, l12, l23);
    const bool c24 = compare_deltas(d23, d34, l23, l34);
    const bool c35 = compare_deltas(d34, d45, l34, l45);

    const float result0 = smart_blur(a0, a1, a2, a3, a4, c02, c13, c24);
    const float result1 = smart_blur(a1, a2, a3, a4, a5, c13, c24, c35);

    ao_samples0[top_index]         = result0;
    ao_samples0[top_index + 16]    = result1;
}

// We essentially want 5 weights:  4 for each low-res pixel and 1 to blend in when none of the 4 really
// match.  The filter strength is 1 / DeltaZTolerance.  So a tolerance of 0.01 would yield a strength of 100.
// Note that a perfect match of low to high depths would yield a weight of 10^6, completely superceding any
// noise filtering.  The noise filter is intended to soften the effects of shimmering when the high-res depth
// buffer has a lot of small holes in it causing the low-res depth buffer to inaccurately represent it.
float bilateral_upsample(float hi_depth, float hi_ao, vec4 lo_depth, vec4 lo_ao) {
    const vec4 weights = vec4(9.0, 3.0, 1.0, 3.0) / (abs(hi_depth - lo_depth) + upsample_tolerance);
    const float total = dot(weights, vec4(1.0)) + noise_filter_weight;
    const float sum = dot(lo_ao, weights) + noise_filter_weight;
    return hi_ao * sum / total;
}

void main() {
    const vec2 inv_lo_res = 1.0 / vec2(textureSize(in_lo_depth, 0).xy);
    const vec2 inv_hi_res = 1.0 / vec2(textureSize(in_hi_depth, 0).xy);

    prefetch(gl_LocalInvocationID.x << 1 | gl_LocalInvocationID.y << 5, ivec2(gl_GlobalInvocationID.xy + gl_LocalInvocationID.xy - 2) * inv_lo_res);
    barrier();

    // Goal:  End up with a 9x9 patch that is blurred so we can upsample.  Blur radius is 2 pixels, so start with 13x13 area.
    // Horizontally blur the pixels.    13x13 -> 9x13
    if(gl_LocalInvocationIndex < 39) {
        horizontal_blur((gl_LocalInvocationIndex / 3) * 16 + (gl_LocalInvocationIndex % 3) * 3);
    }
    barrier();

    // Vertically blur the pixels.        9x13 -> 9x9
    if(gl_LocalInvocationIndex < 45) {
        vertical_blur((gl_LocalInvocationIndex / 9) * 32 + gl_LocalInvocationIndex % 9);
    }
    barrier();

    // Bilateral upsample
    const uint id = gl_LocalInvocationID.x + gl_LocalInvocationID.y * 16;
    const vec4 lo_samples = vec4(ao_samples0[id + 16], ao_samples0[id + 17], ao_samples0[id + 1], ao_samples0[id]);

    // We work on a quad of pixels at once because then we can gather 4 each of high and low-res depth values
    const vec2 uv = gl_GlobalInvocationID.xy * inv_lo_res;

#ifdef COMBINE_HIGH
    const vec4 hi_samples  = textureGather(in_hi_ao, uv);
#else
    const vec4 hi_samples = vec4(1.0);
#endif
    const vec4 lo_depths = textureGather(in_lo_depth, uv);
    const vec4 hi_depths = textureGather(in_hi_depth, uv);

    const ivec2 out_coord = ivec2(gl_GlobalInvocationID.xy) << 1;
    imageStore(out_ao, out_coord + ivec2(-1,  0), vec4(bilateral_upsample(hi_depths.x, hi_samples.x, lo_depths.xyzw, lo_samples.xyzw)));
    imageStore(out_ao, out_coord + ivec2( 0,  0), vec4(bilateral_upsample(hi_depths.y, hi_samples.y, lo_depths.yzwx, lo_samples.yzwx)));
    imageStore(out_ao, out_coord + ivec2( 0, -1), vec4(bilateral_upsample(hi_depths.z, hi_samples.z, lo_depths.zwxy, lo_samples.zwxy)));
    imageStore(out_ao, out_coord + ivec2(-1, -1), vec4(bilateral_upsample(hi_depths.w, hi_samples.w, lo_depths.wxyz, lo_samples.wxyz)));
}


#ifndef BILATERAL_SLANG
#define BILATERAL_SLANG

#include "utils.slang"

struct BilateralUpsampleParams {
    float spatial_sigma = 1.5;
    float depth_phi = 0.05;
    float normal_phi = 16.0;
};

float4 bilateral_upsample(Sampler2D<float4> in_color, Sampler2D<float> in_depth, Sampler2D<uint> in_rt1, float4x4 inv_proj, uint2 coord, float linear_depth, float3 normal, BilateralUpsampleParams params = {}) {

    const uint2 size = image_size(in_depth);
    const int2 low_size = int2(image_size(in_color));
    const float2 scale = float2(size) / float2(low_size);
    const int2 center = int2(float2(coord) / scale);

    float4 acc = float4(0.0);
    float total_weight = 0.0;

    float4 fallback = float4(0.0);
    float fallback_dist = constants.max_float;

    for(int y = -1; y <= 1; ++y) {
        for(int x = -1; x <= 1; ++x) {
            const int2 low_coord = center + int2(x, y);
            if(any(low_coord < 0) || any(low_coord >= low_size)) {
                continue;
            }

            const uint2 rep_coord = min(uint2(float2(low_coord) * scale), size - 1);
            const float rep_depth = in_depth[rep_coord];
            if(is_depth_far(rep_depth)) {
                continue;
            }

            const float4 color = in_color[low_coord];

            const float2 offset = float2(rep_coord) - float2(coord);
            const float dist2 = dot(offset, offset);

            if(dist2 < fallback_dist) {
                fallback_dist = dist2;
                fallback = color;
            }

            const float rep_linear_depth = linearize_depth(rep_depth, inv_proj);
            const float3 rep_normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[rep_coord])).normal;

            const float weight =
                exp(-dist2 / (2.0 * params.spatial_sigma * params.spatial_sigma)) *
                exp(-abs(rep_linear_depth - linear_depth) / (params.depth_phi * abs(linear_depth))) *
                pow(saturate(dot(rep_normal, normal)), params.normal_phi
            );

            acc += color * weight;
            total_weight += weight;
        }
    }

    return total_weight > constants.epsilon ? acc / total_weight : fallback;
}

#endif // BILATERAL_SLANG

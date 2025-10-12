#include "lib/utils.slang"

// Based on MiniEngine's implementation
// see: https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/AoRenderCS.hlsli

// 16x16 cache size:  the 8x8 in the center forms the area of focus with the 4-pixel perimeter used for gathering.
#define THREAD_COUNT 8
#define CENTER_OFFSET 4
#define COORDS_MUL 1

#define TILE_DIM (THREAD_COUNT * 2)


[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
RWTexture2D<float> out_ao;

[[vk::binding(2)]]
cbuffer Params_Inline {
    float4 inv_thickness[3];
    float4 weights[3];
};




groupshared float depth_samples[TILE_DIM * TILE_DIM];

float test_sample_pair(float front_depth, float inv_range, uint base, int offset) {
    // "Disocclusion" measures the penetration distance of the depth sample within the sphere.
    // Disocclusion < 0 (full occlusion) -> the sample fell in front of the sphere
    // Disocclusion > 1 (no occlusion) -> the sample fell behind the sphere
    const float disocclusion_1 = depth_samples[base + offset] * inv_range - front_depth;
    const float disocclusion_2 = depth_samples[base - offset] * inv_range - front_depth;

    const float reject_fadeoff = -1.0;
    const float pseudo_disocclusion_1 = saturate(reject_fadeoff * disocclusion_1);
    const float pseudo_disocclusion_2 = saturate(reject_fadeoff * disocclusion_2);

    return
        clamp(disocclusion_1, pseudo_disocclusion_2, 1.0) +
        clamp(disocclusion_2, pseudo_disocclusion_1, 1.0) -
        pseudo_disocclusion_1 * pseudo_disocclusion_2;
}

float test_samples(uint center_index, int x, int y, float inv_depth, float inv_thickness) {
    x *= COORDS_MUL;
    y *= COORDS_MUL;

    const float inv_range = inv_thickness * inv_depth;
    const float front_depth = inv_thickness - 0.5;

    if(y == 0) {
        // Axial
        return 0.5 * (
            test_sample_pair(front_depth, inv_range, center_index, x) +
            test_sample_pair(front_depth, inv_range, center_index, x * TILE_DIM)
        );
    } else if(x == y) {
        // Diagonal
        return 0.5 * (
            test_sample_pair(front_depth, inv_range, center_index, x * TILE_DIM - x) +
            test_sample_pair(front_depth, inv_range, center_index, x * TILE_DIM + x)
        );
    } else {
        // L-Shaped
        return 0.25 * (
            test_sample_pair(front_depth, inv_range, center_index, y * TILE_DIM + x) +
            test_sample_pair(front_depth, inv_range, center_index, y * TILE_DIM - x) +
            test_sample_pair(front_depth, inv_range, center_index, x * TILE_DIM + y) +
            test_sample_pair(front_depth, inv_range, center_index, x * TILE_DIM - y)
        );
    }
}

[shader("compute")]
[numthreads(THREAD_COUNT, THREAD_COUNT)]
void comp_main() {
    const float2 image_size = float2(image_size(in_depth).xy);
    //const float2 image_size = float2(semantics.work_group_count.xy * semantics.work_group_size.xy);
    const uint2 center_coord = uint2(semantics.global_id.xy + semantics.local_id.xy) - CENTER_OFFSET;
    const float2 center_uv = float2(center_coord) / image_size;

    const float4 depths = in_depth.Gather(center_uv);

    const uint dst_index = semantics.local_id.x * 2 + semantics.local_id.y * 2 * TILE_DIM;
    depth_samples[dst_index]                = depths.w;
    depth_samples[dst_index + 1]            = depths.z;
    depth_samples[dst_index + TILE_DIM]     = depths.x;
    depth_samples[dst_index + TILE_DIM + 1] = depths.y;

    GroupMemoryBarrierWithGroupSync();

    const uint index = semantics.local_id.x + semantics.local_id.y * TILE_DIM + (TILE_DIM / 4) * TILE_DIM + (TILE_DIM / 4);
    const float inv_depth = 1.0 / depth_samples[index];

    float ao = 0.0;

#if 0
    ao += weights[0].x * test_samples(index, 1, 0, inv_depth, inv_thickness[0].x);
    ao += weights[0].y * test_samples(index, 2, 0, inv_depth, inv_thickness[0].y);
    ao += weights[0].z * test_samples(index, 3, 0, inv_depth, inv_thickness[0].z);
    ao += weights[0].w * test_samples(index, 4, 0, inv_depth, inv_thickness[0].w);
    ao += weights[1].x * test_samples(index, 1, 1, inv_depth, inv_thickness[1].x);
    ao += weights[2].x * test_samples(index, 2, 2, inv_depth, inv_thickness[2].x);
    ao += weights[2].w * test_samples(index, 3, 3, inv_depth, inv_thickness[2].w);
    ao += weights[1].y * test_samples(index, 1, 2, inv_depth, inv_thickness[1].y);
    ao += weights[1].z * test_samples(index, 1, 3, inv_depth, inv_thickness[1].z);
    ao += weights[1].w * test_samples(index, 1, 4, inv_depth, inv_thickness[1].w);
    ao += weights[2].y * test_samples(index, 2, 3, inv_depth, inv_thickness[2].y);
    ao += weights[2].z * test_samples(index, 2, 4, inv_depth, inv_thickness[2].z);
#else
    // 36 samples:  sample every-other cell in a checker board pattern
    ao += weights[0].y * test_samples(index, 2, 0, inv_depth, inv_thickness[0].y);
    ao += weights[0].w * test_samples(index, 4, 0, inv_depth, inv_thickness[0].w);
    ao += weights[1].x * test_samples(index, 1, 1, inv_depth, inv_thickness[1].x);
    ao += weights[2].x * test_samples(index, 2, 2, inv_depth, inv_thickness[2].x);
    ao += weights[2].w * test_samples(index, 3, 3, inv_depth, inv_thickness[2].w);
    ao += weights[1].z * test_samples(index, 1, 3, inv_depth, inv_thickness[1].z);
    ao += weights[2].z * test_samples(index, 2, 4, inv_depth, inv_thickness[2].z);
#endif

    out_ao[semantics.global_id.xy] = ao;
}


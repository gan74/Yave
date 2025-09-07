#include "lib/utils.slang"


[[vk::binding(0)]]
RWTexture2D out_image;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<uint> in_rt1;

[[vk::binding(4)]]
Sampler2D in_gi;

[[vk::binding(5)]]
cbuffer Params {
    uint iterations;
}



[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(in_depth);

    if(!all(coord < size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        out_image[coord] = 0.0;
        return;
    }

    const float2 uv = (coord + 0.5) / size;
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 normal = decode_gbuffer({float4(0.0), in_rt1[coord]}).normal;

    const float view_dist = length(world_pos - camera.position);


    float total = 1.0;
    float3 acc = in_gi[coord].rgb;

    const float radius = 60.0;
    for(float r = 1.0; r < radius; r *= 1.5) {
        for(uint p = 0; p != 6; ++p) {
            const float phi = p / 6.0 * pi * 2.0;
            const float2 sample_offset = float2(cos(phi + r), sin(phi + r)) * r;
            const float2 sample_uv = uv + (sample_offset / size);
            if(any(saturate(sample_uv) != sample_uv)) {
                continue;
            }

            const float sample_depth = in_depth.SampleLevel(sample_uv, 0.0);
            const float3 sample_world_pos = unproject(sample_uv, sample_depth, camera.curr.inv_view_proj);
            const float3 sample_normal = decode_gbuffer({float4(0.0), in_rt1.SampleLevel(sample_uv, 0.0)}).normal;

            if(is_depth_far(sample_depth)) {
                continue;
            }

            if(dot(normal, sample_normal) < 0.8) {
                continue;
            }

            if(length(sample_world_pos - world_pos) * 100.0 < view_dist) {
                continue;
            }

            total += 1.0;
            acc += in_gi.SampleLevel(sample_uv, 0.0).rgb;
        }
    }

    out_image[coord] = float4(acc / total, 0);
}



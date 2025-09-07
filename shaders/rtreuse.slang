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
    uint seed;
    uint sample_count;
}




float wrap_uv(float x) {
    if(x < 0.0) {
        x = -x;
    }
    if(x > 1.0) {
        x = 1.0 - frac(x);
    }
    return x;
}

float2 wrap_uv(float2 v) {
    return float2(wrap_uv(v.x), wrap_uv(v.y));
}



static uint normal_rejection = 0;
static uint depth_rejection = 0;

float4 take_sample(float2 sample_uv, float3 world_pos, float3 normal, bool reject_depth, bool reject_normal) {
    const float sample_depth = in_depth.SampleLevel(sample_uv, 0.0);
    const float3 sample_world_pos = unproject(sample_uv, sample_depth, camera.curr.inv_view_proj);
    const float3 sample_normal = decode_gbuffer({float4(0.0), in_rt1.SampleLevel(sample_uv, 0.0)}).normal;

    if(is_depth_far(sample_depth)) {
        return float4(0.0);
    }

    if(reject_normal && dot(normal, sample_normal) < 0.8) {
        ++normal_rejection;
        return float4(0.0);
    }

#if 0
    const float view_dist = length(world_pos - camera.position);
    if(reject_depth && length(sample_world_pos - world_pos) > view_dist * 0.1) {
        ++depth_rejection;
        return float4(0.0);
    }
#else
    const float3 to_point = sample_world_pos - world_pos;
    const float3 sample_dir = normalize(to_point);
    if(reject_depth && abs(dot(normal, sample_dir)) > 0.1) {
        ++depth_rejection;
        return float4(0.0);
    }
#endif

    return float4(in_gi.SampleLevel(sample_uv, 0.0).rgb, 1.0);
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

#if 0
    float4 acc = 0.0;
    const int kernel_size = 7;
    for(int x = -kernel_size; x <= kernel_size; ++x) {
        for(int y = -kernel_size; y <= kernel_size; ++y) {
            const float2 sample_uv = wrap_uv(uv + (float2(x, y) / size));
            acc += take_sample(sample_uv, world_pos, normal, true, true);
        }
    }
#else
    float4 acc = float4(in_gi.SampleLevel(uv, 0.0).rgb, 1.0);

    const float random = float((hash(semantics.global_index) ^ seed) & 0x0FFFFFF) / 0x0FFFFFF;
    const float random_angle = random * pi * 2.0;

    const float step = 1.0 / sample_count;
    for(float t = 0.0; t < 1.0; t += step) {
        const float r = lerp(2.0, 32.0, t);
        const float phi = lerp(0.0, pi * 2.0 * 5.0, t);
        const float2 sample_offset = float2(cos(phi + random_angle), sin(phi + random_angle)) * r;
        const float2 sample_uv = wrap_uv(uv + (sample_offset / size));

        acc += take_sample(sample_uv, world_pos, normal, true, true);
    }
#endif

    out_image[coord] = float4(acc.rgb / max(acc.a, 0.01), acc.a);

    // out_image[coord] = float4(heat_spectrum(acc.a / sample_count), 0);
    // out_image[coord] = float4(heat_spectrum(depth_rejection / float(sample_count)), 0);
}



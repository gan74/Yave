

#include "lib/utils.slang"

#define TAA_CLIP
// #define TAA_DENOISE


[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D in_prev_color;

[[vk::binding(2)]]
Sampler2D<float2> in_motion;

[[vk::binding(3)]]
Sampler2D<float2> in_prev_motion;

[[vk::binding(4)]]
Sampler2D<float> in_depth;

[[vk::binding(5)]]
ConstantBuffer<Camera> camera;

[[vk::binding(6)]]
cbuffer Params {
    TAAFeatureBits flags;
    float clamping_range;
    float anti_flicker_strength;
}


[[vk::binding(7)]]
RWTexture2D out_color;






static float3 local_neighbourhood[9];


static const int2 offsets[9] = int2[9](
    int2(-1, -1),
    int2(0, -1),
    int2(1, -1),
    int2(-1, 0),
    int2(0, 0),
    int2(1, 0),
    int2(-1, 1),
    int2(0, 1),
    int2(1, 1)
);






void load_local_data() {
    for(uint i = 0; i != 9; ++i) {
        local_neighbourhood[i] = in_color[semantics.global_id.xy + offsets[i]].rgb;
    }
}

int2 find_nearest_pixel() {
    float depth = 0.0;
    int2 offset = int2(0);
    for(uint i = 0; i != 9; ++i) {
        const float curr_depth = in_depth[semantics.global_id.xy + offsets[i]];
        if(curr_depth > depth) {
            depth = curr_depth;
            offset = offsets[i];
        }
    }

    return offset;
}

void compute_variance_min_max(inout float3 aabb_min, inout float3 aabb_max) {
    float3 m1 = float3(0.0);
    float3 m2 = float3(0.0);
    for(int i = 0; i != 9; ++i) {
        const float3 color = local_neighbourhood[i];
        m1 += color;
        m2 += sqr(color);
    }

    const float inv_sample_count = 1.0 / 9.0;
    const float gamma = 1.0;

    const float3 mu = m1 * inv_sample_count;
    const float3 sigma = sqrt(abs((m2 * inv_sample_count) - sqr(mu)));
    aabb_min = mu - gamma * sigma;
    aabb_max = mu + gamma * sigma;
}

float copy_sign(float x, float sign) {
    if(sign < 0.0) {
        return -abs(x);
    }
    return abs(x);
}

float clip_bbox(float3 box_min, float3 box_max, float3 prev, float3 curr) {
    float3 dir = curr - prev;

    // Make sure dir isn't zero
    dir.x = abs(dir.x) < epsilon ? copy_sign(epsilon, dir.x) : dir.x;
    dir.y = abs(dir.y) < epsilon ? copy_sign(epsilon, dir.y) : dir.y;
    dir.z = abs(dir.z) < epsilon ? copy_sign(epsilon, dir.z) : dir.z;

    const float3 inv_dir = 1.0 / dir;

    const float3 t0 = (box_min - prev) * inv_dir;
    const float3 t1 = (box_max - prev) * inv_dir;

    return component_max(min(t0, t1));
}

float3 sample_prev(float2 uv) {
    return in_prev_color.SampleLevel(uv, 0).rgb;
}


// Implementation from https://github.com/tippesi/Atlas-Engine/blob/master/data/shader/taa.csh
[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    load_local_data();

    const int2 offset = find_nearest_pixel();

    const float3 tl = local_neighbourhood[0];
    const float3 tc = local_neighbourhood[1];
    const float3 tr = local_neighbourhood[2];
    const float3 ml = local_neighbourhood[3];
    const float3 mc = local_neighbourhood[4];
    const float3 mr = local_neighbourhood[5];
    const float3 bl = local_neighbourhood[6];
    const float3 bc = local_neighbourhood[7];
    const float3 br = local_neighbourhood[8];

    // 3x3 box pattern
    const float3 box_min = min(tl, min(tc, min(tr, min(ml, min(mc, min(mr, min(bl, min(bc, br))))))));
    const float3 box_max = max(tl, max(tc, max(tr, max(ml, max(mc, max(mr, max(bl, max(bc, br))))))));

    // 5 sample cross pattern
    const float3 cross_min = min(tc, min(ml, min(mc, min(mr, bc))));
    const float3 cross_max = max(tc, max(ml, max(mc, max(mr, bc))));

    // Average both bounding boxes to get a more rounded shape
    float3 local_neighbourhood_min = 0.5 * (box_min + cross_min);
    float3 local_neighbourhood_max = 0.5 * (box_max + cross_max);

    const float3 average = 0.5 * (local_neighbourhood_min + local_neighbourhood_max);

    compute_variance_min_max(local_neighbourhood_min, local_neighbourhood_max);

    const uint2 coord = semantics.global_id.xy;
    const int2 velocity_coord = coord + offset;
    const float2 velocity = in_motion[velocity_coord];
    const float2 last_velocity = in_prev_motion[coord];

    const float velocity_blend = saturate(600.0 * max(length(velocity), length(last_velocity)));

    const float2 uv = (float2(coord) + float2(0.5)) / image_size(in_color) + velocity;

    float3 prev_color = sample_prev(uv);
    float3 curr_color = mc;

    const float curr_lum = luminance(curr_color);
    const float prev_lum = luminance(prev_color);

    const float prev_contrast = saturate(abs(curr_lum - prev_lum) / component_max(float3(0.2, curr_lum, prev_lum)));
    const float anti_flicker = lerp(0.0, anti_flicker_strength, 1.0 - velocity_blend);
    float range = clamping_range;

    if((flags & TAAFeatureBits::DenoiseBit) != 0) {
        range += anti_flicker;
    } else {
        range += lerp(0.0, anti_flicker, prev_contrast);
    }

    local_neighbourhood_min = average - range * (average - local_neighbourhood_min);
    local_neighbourhood_max = average + range * (local_neighbourhood_max - average);

    if((flags & TAAFeatureBits::ClampingBit) != 0) {
        const float clip_blend = clip_bbox(local_neighbourhood_min, local_neighbourhood_max, prev_color, curr_color);
        const float adj_clip_blend = saturate(clip_blend);
        prev_color = lerp(prev_color, curr_color, adj_clip_blend);
    } else {
        prev_color = clamp(prev_color, local_neighbourhood_min, local_neighbourhood_max);
    }

    const float min_velocity_blend = 0.05;
    const float max_velocity_blend = 0.2;
    const float blend_factor = (any(saturate(uv) != uv)) ? 1.0 : lerp(min_velocity_blend, max_velocity_blend, velocity_blend);

    const float prev_weight = (1.0 - blend_factor) / (1.0 + luminance(prev_color));
    const float curr_weight =  blend_factor / (1.0 + luminance(curr_color));

    const float3 resolved = abs(((prev_color * prev_weight) + (curr_color * curr_weight)) / (prev_weight + curr_weight));

    if (any(isnan(resolved))) {
        out_color[coord] = float4(1.0, 0.0, 0.0, 1.0);
    } else {
        out_color[coord] = float4(resolved, 1.0);
    }

    //out_color[coord] = float4(lerp(in_color[coord].rgb, in_prev_color[coord].rgb, 0.9), 1.0);
}


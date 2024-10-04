#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D in_prev_color;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<float> in_prev_depth;

[[vk::binding(4)]]
Sampler2D<float2> in_motion;

[[vk::binding(5)]]
ConstantBuffer<Camera> camera;



[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);

    const float2 size = float2(image_size(in_color).xy);
    const float2 inv_size = 1.0 / size;
    const float y_ratio = size.y * inv_size.x;

    const float2 motion = in_motion[coord];
    const float2 uv = semantics.frag_coord.xy * inv_size;
    const float2 prev_uv = uv + motion;

    const float4 current = in_color[coord];

    if(any(saturate(prev_uv) != prev_uv)) {
        return current;
    }

    const float depth = in_depth.Sample(uv);
    const float prev_depth = in_prev_depth.Sample(prev_uv);

    if(is_depth_far(depth) || is_depth_far(prev_depth)) {
        return current;
    }

    const float3 prev_world_pos = unproject(prev_uv, prev_depth, camera.prev.inv_view_proj);
    const float3 reproj = project(prev_world_pos, camera.cur.view_proj);

    if(any(abs(reproj.xy - uv) > inv_size)) {
        return current;
    }

    const float4 prev = in_prev_color.SampleLevel(prev_uv, 0.0);
    return lerp(current, prev, 0.75);
}

#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D in_prev;

[[vk::binding(2)]]
Sampler2D<float2> in_motion;



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

    if(all(saturate(prev_uv) == prev_uv)) {
        const float4 prev = in_prev.SampleLevel(prev_uv, 0.0);
        return lerp(current, prev, 0.9);
    }

    return current;
}

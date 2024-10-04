#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D in_prev_color;

[[vk::binding(2)]]
Sampler2D<float2> in_motion;

[[vk::binding(3)]]
Sampler2D<uint> in_mask;

[[vk::binding(4)]]
ConstantBuffer<Camera> camera;


[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);

    const float4 current = in_color[coord];
    if(in_mask[coord] != 0) {
        return current;
    }

    const float2 size = float2(image_size(in_color).xy);
    const float2 inv_size = 1.0 / size;
    const float2 uv = semantics.frag_coord.xy * inv_size;
    const float2 prev_uv = uv + in_motion[coord];

    const float4 prev = in_prev_color.Sample(prev_uv);
    return lerp(current, prev, 0.75);
}

#include "../lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    return float4(linear_to_sRGB(in_color[int2(semantics.frag_coord.xy)].rgb), 1.0);
}

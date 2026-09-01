#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
RWTexture2D<float4> out_color;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const float2 image_size = float2(image_size(out_color).xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;

    const float4 color = in_color.SampleLevel(uv, 0.0);
    out_color[semantics.global_id.xy] = float4(color.rgb, 1.0);
}



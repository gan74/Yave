#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
RWTexture2D<float4> out_color;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const float2 uv = float2(semantics.global_id.xy) / float2(semantics.work_group_count.xy * semantics.work_group_size.xy);

    const float4 color = in_color.SampleLevel(uv, 0.0);
    out_color[semantics.global_id.xy] = float4(color.rgb, 1.0);
}



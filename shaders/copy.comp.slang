#include "lib/utils.slang"

[[vk::binding(0, 0)]]
Sampler2D in_color;

[[vk::binding(1, 0)]]
RWTexture2D out_color;


[shader("compute")]
[numthreads(8, 8)]
void main(uint3 thread_id : SV_DispatchThreadID) {
    const float2 uv = float2(thread_id.xy) / float2(workgroup_count().xy);

    const float4 color = in_color.SampleLevel(uv, 0.0);
    out_color[thread_id.xy] = float4(color.rgb, 1.0);
}


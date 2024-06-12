import "glsl";

#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
RWTexture2D out_color;

[shader("compute")]
[numthreads(8, 8)]
void main() {
    const float2 uv = float2(gl_GlobalInvocationID.xy) / float2(gl_NumWorkGroups.xy * gl_WorkGroupSize.xy);

    const float4 color = in_color.SampleLevel(uv, 0.0);
    out_color[gl_GlobalInvocationID.xy] = float4(color.rgb, 1.0);
}



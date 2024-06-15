import "glsl";

#include "../lib/utils.slang"

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D in_depth;

[[vk::binding(2)]]
RWTexture2D out_color;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    const float4 color = in_color[coord];
    const float depth = in_depth[coord].r;

    out_color[coord] = (is_depth_OOB(depth) ? float4(0.0) : color);
}


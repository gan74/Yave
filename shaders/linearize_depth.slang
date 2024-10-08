#include "lib/utils.slang"

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
RWTexture2D<float> out_depth;

[shader("compute")]
[numthreads(8, 8)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);

    const float depth = in_depth[coord];
    out_depth[coord] = linearize_depth(depth, camera.inv_proj);
}


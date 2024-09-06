#include "lib/utils.slang"

// -------------------------------- I/O --------------------------------

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
RWStructuredBuffer<GIProbe> out_probes;



// -------------------------------- MAIN --------------------------------

[shader("compute")]
[numthreads(16, 16)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);
    const float2 image_size = float2(image_size(in_depth).xy);
    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;

    const float depth = in_depth[coord];

    const float3 world_pos = unproject(uv, depth, camera.inv_view_proj);
    const float3 view_dir = normalize(camera.position - world_pos);

    if(semantics.local_index == (semantics.work_group_size.x + 1) * (semantics.work_group_size.y / 2)) {
        out_probes[semantics.work_group_index] = {world_pos + view_dir * 0.2};
    }
}


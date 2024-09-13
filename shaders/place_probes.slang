#include "lib/utils.slang"
#include "lib/gi.slang"

// -------------------------------- I/O --------------------------------

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
RWStructuredBuffer<GIProbe> out_probes;



// -------------------------------- MAIN --------------------------------

float3 compute_world_pos(int2 coord) {
    const float2 image_size = float2(image_size(in_depth).xy);
    const float2 uv = (coord + 0.5) / image_size;

    const float depth = in_depth[coord];

    const float3 world_pos = unproject(uv, depth, camera.inv_view_proj);
    return world_pos;
}


[shader("compute")]
[numthreads(PROBE_PIXEL_SIZE, PROBE_PIXEL_SIZE)]
void comp_main() {
    const int2 coord = int2(semantics.global_id.xy);

    const float3 world_pos = compute_world_pos(coord);
    const float3 world_pos_x = compute_world_pos(coord + int2(1, 0));
    const float3 world_pos_y = compute_world_pos(coord + int2(0, 1));

    const float3 normal = normalize(cross(world_pos - world_pos_y, world_pos - world_pos_x));

    if(semantics.local_index == (semantics.work_group_size.x + 1) * (semantics.work_group_size.y / 2)) {
        out_probes[semantics.work_group_index].position = world_pos + normal * 0.2;
        out_probes[semantics.work_group_index].normal = normal;
    }
}


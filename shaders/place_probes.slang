#include "lib/ddgi.slang"

[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
RWTexture3D<uint> out_grid;

[[vk::binding(4)]]
cbuffer Params {
    uint frame_id;
};



[shader("compute")]
[numthreads(8, 8)]
void comp() {
#if 0
    const uint2 size = image_size(in_depth);
    const uint2 coord = semantics.global_id.xy;

    if(any(coord >= size)) {
        return;
    }

    const float2 px_center = float2(coord) + 0.5;
    const float2 in_uv = px_center / float2(size);

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        return;
    }

    const SurfaceInfo surface = decode_gbuffer({
        float4(0.0),
        in_rt1[coord],
    });

    const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);
    const int3 probe_id = ddgi.compute_grid_coord(world_pos, surface.normal);

    if(!ddgi.in_range(world_pos, camera.position)) {
        return;
    }

    out_grid[probe_id] = frame_id;
#endif
}



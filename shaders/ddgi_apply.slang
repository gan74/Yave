#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D<float3> out_gi;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
Sampler2D<float3> in_ddgi_irradiance;

[[vk::binding(5)]]
Sampler2D<float2> in_ddgi_distance;

[[vk::binding(6)]]
Sampler2D<uint> probe_grid;

[[vk::binding(7)]]
cbuffer Params {
    float probe_spacing;
    uint atlas_probe_count;
    uint padding_0;
    uint padding_1;
};



uint compute_probe_index(int3 coord) {
    return uint(
        coord.x * ddgi.grid_size * ddgi.grid_size +
        coord.y * ddgi.grid_size +
        coord.z
    );
}

[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(out_gi).xy;
    if(any(coord >= size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        out_gi[coord] = float3(0.0);
        return;
    }

    const float2 uv = (float2(coord) + 0.5) / float2(size);
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 normal = decode_gbuffer(RawGBuffer(float4(0.0), in_rt1[coord])).normal;

    const float3 gi = sample_irradiance(
        world_pos,
        normal,
        camera.position,
        probe_spacing,
        atlas_probe_count,
        probe_grid,
        in_ddgi_irradiance,
        in_ddgi_distance
    );

    out_gi[coord] = gi;

    /*const int3 grid_coord = ddgi.compute_grid_coord(world_pos, probe_spacing);
    const uint grid_probe_index = compute_probe_index(grid_coord);
    out_gi[coord] = identifying_color(grid_probe_index) * 0.1;*/
    
}

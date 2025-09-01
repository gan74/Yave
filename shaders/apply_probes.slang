#include "lib/ddgi.slang"
#include "lib/rt.slang"


[[vk::binding(0)]]
RWTexture2D out_image;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
Sampler2D in_probes;

[[vk::binding(5)]]
Sampler3D<uint> in_grid;

[[vk::binding(6)]]
cbuffer Params {
    uint frame_id;
};




float3 hemisphere_sample(uint i, uint count) {
    const float theta = 2.0 * pi * i * golden;
    const float phi = acos(1.0 - 2.0 * i / float(count)) * 0.5;
    return float3(float2(cos(theta), sin(theta)) * sin(phi), cos(phi));
}


[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 image_size = image_size(out_image);

    if(any(coord > image_size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        out_image[coord] = float4(0.0);
        return;
    }


    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const SurfaceInfo surface = decode_gbuffer({float4(0.0), in_rt1[coord]});

    const int3 probe_id = ddgi.compute_grid_coord(world_pos, surface.normal);
    const uint probe_index = ddgi.compute_probe_index(probe_id);
    const uint2 atlas_coord = ddgi.compute_atlas_coord(probe_index);

    if(!ddgi.in_range(world_pos, camera.position)) {
        out_image[coord] = float4(0.0);
        return;
    }

    if(in_grid[probe_id] != frame_id) {
        out_image[coord] = float4(0, 1, 0, 0);
        return;
    }

    const float3 probe_pos = ddgi.compute_world_coord(probe_id, camera.position);
    if(dot((probe_pos - world_pos), surface.normal)  < 0.0) {
        out_image[coord] = float4(1, 0, 0, 0);
        return;
    }


    float total = 0.0;
    float3 irr = float3(0.0);
    for(uint u = 0; u != ddgi.probe_size; ++u) {
        for(uint v = 0; v != ddgi.probe_size; ++v) {
            const float3 dir = octahedron_decode((float2(u, v) + 0.5) / ddgi.probe_size);
            const float NoL = dot(dir, surface.normal);
            if(NoL > 0.0) {
                total += NoL;
                irr += in_probes[uint2(u, v) + atlas_coord * ddgi.probe_size].rgb * NoL;
            }
        }
    }

    out_image[coord] = float4(irr / total, 1.0);
    out_image[coord] = float4(identifying_color(probe_index), 1.0);
}

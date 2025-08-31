#include "lib/gi.slang"
#include "lib/rt.slang"

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D<uint> in_rt1;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
Sampler2D in_probes;

[[vk::binding(4)]]
RWTexture2D out_image;

[[vk::binding(5)]]
RaytracingAccelerationStructure tlas;




static const int2 offsets[9] = int2[9](
    int2(-1, -1),
    int2(0, -1),
    int2(1, -1),
    int2(-1, 0),
    int2(0, 0),
    int2(1, 0),
    int2(-1, 1),
    int2(0, 1),
    int2(1, 1)
);

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
        return;
    }

    const float2 uv = (semantics.global_id.xy + 0.5) / image_size;
    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const SurfaceInfo surface = decode_gbuffer({float4(0.0), in_rt1[coord]});

    const int3 probe_id = gi.compute_grid_coord(world_pos, surface.normal);
    const uint probe_index = gi.compute_probe_index(probe_id);
    const uint2 atlas_coord = gi.compute_atlas_coord(probe_index);

    const float3 probe_pos = gi.compute_world_coord(probe_id, camera.position);
    const float3 to_probe = probe_pos - world_pos;
    const float probe_dist = length(to_probe);
    const float3 probe_dir = to_probe / probe_dist;

    /*{
        float4 debug_color = float4(0.0);
        if(trace_inline(tlas, world_pos, probe_dir, 0.01, probe_dist).hit) {
            debug_color.r = 1.0;
        }

        const float2 to_probe_uv = saturate(octahedron_encode(-probe_dir));
        const uint2 probe_coord = uint2(round(to_probe_uv * gi.probe_size));

        float dist = 0.0;
        for(uint i = 0; i != 9; ++i) {
            dist = max(dist, in_probes[(probe_coord + offsets[i]) % gi.probe_size + atlas_coord * gi.probe_size].w);
        }

        if(dist < 0.0) {
            debug_color.b = 1.0;
        } else if(dist < probe_dist) {
            debug_color.g = dist / 2.0;
        }

        if(any(debug_color > 0.0)) {
            out_image[coord] = debug_color;
            return;
        }
    }*/


    float total = 0.0;
    float3 irr = float3(0.0);
    for(uint u = 0; u != gi.probe_size; ++u) {
        for(uint v = 0; v != gi.probe_size; ++v) {
            const float3 dir = octahedron_decode((float2(u, v) + 0.5) / gi.probe_size);
            const float NoL = dot(dir, surface.normal);
            if(NoL > 0.0) {
                total += NoL;
                irr += in_probes[uint2(u, v) + atlas_coord * gi.probe_size].rgb * NoL;
            }
        }
    }

    out_image[coord] = float4(irr / total, 1.0);
}

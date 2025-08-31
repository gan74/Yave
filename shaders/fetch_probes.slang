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

    const float3 normal = surface.normal;
    const float3 tangent = build_orthogonal(normal);
    const float3 bitan = cross(normal, tangent);

    float total = 0.0;
    float3 irr = float3(0.0);

    const uint samples = 32;
    for(uint i = 0; i != samples; ++i) {
        const float3 s = hemisphere_sample(i, samples);
        const float3 sample_dir = s.x * tangent + s.y * bitan + s.z * normal;
        const float NoL = dot(sample_dir, normal);

        const float2 probe_uv = saturate(octahedron_encode(sample_dir));

        total += NoL;
        irr += in_probes[uint2(clamp(probe_uv * gi.probe_size, 0, gi.probe_size - 1)) + atlas_coord * gi.probe_size].rgb * NoL;
    }

    out_image[coord] = float4(irr / total, 1.0);
}

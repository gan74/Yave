#include "lib/ddgi.slang"
#include "lib/rt.slang"


[[vk::binding(0)]]
RWTexture2D<float3> out_image;

[[vk::binding(1)]]
Sampler2D<float> in_depth;

[[vk::binding(2)]]
Sampler2D<uint> in_rt1;

[[vk::binding(3)]]
Sampler2D<float3> in_irradiance;

[[vk::binding(4)]]
Sampler2D<float2> in_distance;

[[vk::binding(5)]]
ConstantBuffer<Camera> camera;

/*[[vk::binding(6)]]
RaytracingAccelerationStructure tlas;*/


[shader("compute")]
[numthreads(8, 8)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint2 size = image_size(out_image);

    if(any(coord > size)) {
        return;
    }

    const float depth = in_depth[coord];
    if(is_depth_far(depth)) {
        out_image[coord] = float3(0.0);
        return;
    }

    const float3 world_pos = unproject(((semantics.global_id.xy + 0.5) / size), depth, camera.curr.inv_view_proj);
    if(!ddgi.in_range(world_pos, camera.position)) {
        out_image[coord] = float3(0.0);
        return;
    }

    const float2 inv_atlas_size = 1.0 / image_size(in_irradiance);

    const SurfaceInfo surface = decode_gbuffer({float4(0.0), in_rt1[coord]});
    const float2 sample_coord = octahedron_encode(surface.normal) * ddgi.probe_size_no_border;

    const int3 lo = ddgi.compute_grid_coord_low(world_pos);
    const int3 hi = ddgi.compute_grid_coord_high(world_pos);

    float total = 0.0;
    float3 acc = 0.0;
    for(uint k = 0; k != 8; ++k) {
        const int3 probe_id = int3(
            ((k & 0x01) != 0) ? lo.x : hi.x,
            ((k & 0x02) != 0) ? lo.y : hi.y,
            ((k & 0x04) != 0) ? lo.z : hi.z,
        );

        const int3 probe_pos = ddgi.compute_world_coord(probe_id, camera.position);
        const float3 probe_vec = probe_pos - world_pos;
        const float probe_dist = length(probe_vec);
        const float3 probe_dir = probe_vec / probe_dist;
        const float3 trilinear = saturate(1.0 - abs(probe_vec));

        float weight = 1.0;
        weight *= trilinear.x * trilinear.y * trilinear.z;
        weight *= max(0.0, dot(probe_dir, surface.normal));
        // weight *= sqr(max(epsilon, (dot(probe_dir, surface.normal) + 1.0) * 0.5)) + 0.2;

        /*if(trace_inline(tlas, world_pos, probe_dir, 0.01, max(0.01, probe_dist)).hit) {
            continue;
        }*/

        const uint probe_index = ddgi.compute_probe_index(probe_id);
        const uint2 probe_corner = ddgi.atlas_coord(probe_index) + 1;
        const float2 sample_uv = (probe_corner + sample_coord) * inv_atlas_size;

        const float2 dist_moments = in_distance.SampleLevel(sample_uv, 0.0);
        const float mean = dist_moments.x;
        const float variance = abs(sqr(mean) - dist_moments.y);
        const float cheb_bias = 0.1;
        const float chebyshev = variance / (variance + sqr(max(0.0, probe_dist - mean))) + cheb_bias;
         weight *= (probe_dist <= mean) ? 1.0 : chebyshev;

        float3 irr = in_irradiance.SampleLevel(sample_uv, 0.0);

#ifdef SQ_BLEND
        irr *= irr;
#endif

        total += weight;
        acc += irr * weight;
    }


#ifdef SQ_BLEND
    irr = sqrt(irr);
#endif

    if(total > epsilon) {
        // out_image[coord] = (acc / total) * 0.5 * pi;
        out_image[coord] = acc / total;
    } else {
        out_image[coord] = float3(0.0);
    }
}

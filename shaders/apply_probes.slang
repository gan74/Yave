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


float3 encode_irr(float3 irr) {
    return sqrt(irr);
}

float3 decode_irr(float3 irr) {
    return sqr(irr);
}


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

    const float3 true_world_pos = unproject(((semantics.global_id.xy + 0.5) / size), depth, camera.curr.inv_view_proj);
    if(!ddgi.in_range(true_world_pos, camera.position)) {
        out_image[coord] = float3(0.0);
        return;
    }

    const float2 inv_atlas_size = 1.0 / image_size(in_irradiance);

    const SurfaceInfo surface = decode_gbuffer({float4(0.0), in_rt1[coord]});
    const float2 sample_coord = octahedron_encode(surface.normal) * ddgi.probe_size_no_border;

    const float3 view_dir = normalize(camera.position - true_world_pos);
    const float3 bias = surface.normal * 0.05 + view_dir * 0.05;
    const float3 biased_world_pos = true_world_pos + bias;

    float total = 0.0;
    float3 acc = 0.0;
    for(int kx = -1; kx <= 1; ++kx) {
        for(int ky = -1; ky <= 1; ++ky) {
            for(int kz = -1; kz <= 1; ++kz) {
                float weight = 1.0;

                const int3 probe_id = ddgi.compute_grid_coord(biased_world_pos + float3(kx, ky, kz));
                const int3 probe_pos = ddgi.compute_world_coord(probe_id, camera.position);
                const float3 probe_vec = probe_pos - biased_world_pos;
                const float probe_dist = length(probe_vec);

#if 1
                {
                    const float3 true_probe_vec = probe_pos - true_world_pos;
                    weight *= max(0.0, dot(normalize(true_probe_vec), surface.normal));
                    // weight *= sqr(max(epsilon, (dot(probe_dir, surface.normal) + 1.0) * 0.5)) + 0.2;
                }
#endif

                const uint probe_index = ddgi.compute_probe_index(probe_id);
                const uint2 probe_corner = ddgi.atlas_coord(probe_index) + 1;
#if 1
                {
                    const float2 dist_sample_coord = octahedron_encode(probe_vec) * ddgi.probe_size_no_border;
                    const float2 dist_sample_uv = (probe_corner + dist_sample_coord) * inv_atlas_size;

                    const float2 dist_moments = in_distance.SampleLevel(dist_sample_uv, 0.0);
                    const float mean = dist_moments.x;
                    const float variance = abs(sqr(mean) - dist_moments.y);

                    const float chebyshev = max(0.0, variance / (variance + sqr(max(0.0, probe_dist - mean))) + 0.1);
                    weight *= (probe_dist <= mean) ? 1.0 : (chebyshev * chebyshev * chebyshev);

                    /*if(trace_inline(tlas, world_pos + bias, normalize(probe_vec), 0.01, max(0.01, probe_dist)).hit) {
                        weight = 0.0;
                    }*/
                }
#endif

                const float crush_threshold = 0.2;
                if (weight < crush_threshold) {
                    weight = max(epsilon, weight);
                    weight *= weight * weight * (1.0 / sqr(crush_threshold));
                }

                {
                    const float3 trilinear = saturate(1.0 - abs(probe_vec) / 1.5);
                    weight *= trilinear.x * trilinear.y * trilinear.z;
                }


                const float2 sample_uv = (probe_corner + sample_coord) * inv_atlas_size;
                const float3 irr = in_irradiance.SampleLevel(sample_uv, 0.0);

                total += weight;
                acc += encode_irr(irr) * weight;
            }
        }
    }


    if(total > epsilon) {
        out_image[coord] = decode_irr(acc / total);
    } else {
        out_image[coord] = float3(0.0);
    }
}

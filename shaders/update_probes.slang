#include "lib/ddgi.slang"
#include "lib/rt.slang"


[[vk::binding(0)]]
RWTexture2D<float3> out_irradiance;

[[vk::binding(1)]]
RWTexture2D<float2> out_distance;

[[vk::binding(2)]]
StructuredBuffer<DDGIRayHit> in_hits;

[[vk::binding(3)]]
ConstantBuffer<Camera> camera;

[[vk::binding(4)]]
cbuffer Params {
    float depth_sharpness;
};


uint mirror(uint x) {
    return ddgi.probe_size_border - (x + 1);
}

uint2 wrap_coord(uint2 c) {
    if((c.x == 0) || (c.x == ddgi.probe_size_border - 1)) {
        c.y = mirror(c.y);
    }

    if((c.y == 0) || (c.y == ddgi.probe_size_border - 1)) {
        c.x = mirror(c.x);
    }

    return clamp(c, 1, ddgi.probe_size_border - 2) - 1;
}


[shader("compute")]
[numthreads(ddgi.probe_size_border, ddgi.probe_size_border)]
void comp() {
    const uint probe_index = semantics.global_id.z;
    const int3 probe_id = ddgi.compute_grid_coord(probe_index);
    const int3 probe_pos = ddgi.compute_world_coord(probe_id, camera.position);
    const bool probe_moved = any(probe_pos != ddgi.compute_world_coord(probe_id, camera.prev_position));

    const float2 uv = (wrap_coord(semantics.global_id.xy) + 0.5) / ddgi.probe_size_no_border;
    const float3 tex_dir = octahedron_decode(uv);

    float total = 0.0;
    float3 irr = 0.0;

    float dist_total = 0.0;
    float2 dist = 0.0;

    const uint begin = probe_index * ddgi.probe_rays;
    const uint end = begin + ddgi.probe_rays;
    for(uint i = begin; i != end; ++i) {
        const DDGIRayHit hit = in_hits[i];

        const float3 ray_vec = hit.position - probe_pos;
        const float ray_len = length(ray_vec);

        const float weight = max(0.0, dot(ray_vec / ray_len, tex_dir));

        total += weight;
        irr += hit.radiance * weight;

        const float dist_weight = pow(weight, depth_sharpness);

        dist_total += dist_weight;
        dist += float2(ray_len, sqr(ray_len)) * dist_weight;
    }

    const uint2 coord = ddgi.atlas_coord(probe_index) + semantics.global_id.xy;

#if 0
    const float blend_factor = probe_moved ? 0.0 : 0.98;
    out_irradiance[coord] = lerp(irr / max(constants.epsilon, total), out_irradiance[coord], blend_factor);
    out_distance[coord] = lerp(dist / max(constants.epsilon, dist_total), out_distance[coord], blend_factor);
#else
    out_irradiance[coord] = irr / max(constants.epsilon, total);
    out_distance[coord] = dist / max(constants.epsilon, dist_total) * 0.5;
#endif


    // out_irradiance[coord] = identifying_color(probe_index);
}

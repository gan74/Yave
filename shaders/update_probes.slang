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
    const uint probe_index = semantics.global_id.z;;

    const float2 uv = (wrap_coord(semantics.global_id.xy) + 0.5) / ddgi.probe_size_no_border;
    const float3 tex_dir = octahedron_decode(uv);

    float total = 0.0;
    float3 irr = 0.0;
    float2 dist = 0.0;

    const uint begin = probe_index * ddgi.probe_rays;
    const uint end = begin + ddgi.probe_rays;
    for(uint i = begin; i != end; ++i) {
        const DDGIRayHit hit = in_hits[i];
        const float weight = max(0.0, dot(hit.dir, tex_dir));

        total += weight;
        irr += hit.radiance * weight;
        dist += float2(hit.t, sqr(hit.t)) * weight;
    }

    const uint2 coord = ddgi.atlas_coord(probe_index) + semantics.global_id.xy;
    if(total > epsilon) {
#if 0
        out_irradiance[coord] = lerp(irr / total, out_irradiance[coord], 0.9);
        out_distance[coord] = lerp(dist / total, out_distance[coord], 0.9);
#else
        out_irradiance[coord] = irr / total;
        out_distance[coord] = dist / total;
#endif
    }


    // out_irradiance[coord] = identifying_color(probe_index);
}

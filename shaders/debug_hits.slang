#include "lib/ddgi.slang"

#define LINES



[[vk::binding(0)]]
StructuredBuffer<DDGIRayHit> in_hits;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

#ifdef LINES
[[vk::binding(2)]]
cbuffer Params {
    int3 coord;
};
#endif




struct PixelStageIn {
    float3 normal;
    uint hit_index;
};

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    PixelStageIn vert;
};




[shader("vertex")]
VertexStageOut vert_main() {
#ifdef LINES
    const bool at_probe = (semantics.vertex_index & 0x01) == 0;
    const uint hit_index = semantics.vertex_index / 2;
#else
    const bool at_probe = false;
    const uint hit_index = semantics.vertex_index;
#endif

    const uint probe_index = hit_index / ddgi.probe_rays;
    const uint ray_index = hit_index % ddgi.probe_rays;
    const int3 probe_id = ddgi.compute_grid_coord(probe_index);
    const int3 probe_pos = ddgi.compute_world_coord(probe_id, camera.position);

    const DDGIRayHit hit = in_hits[hit_index];
    const float3 position = at_probe ? probe_pos : (hit.dir * max(0.0, hit.t - 0.001) + probe_pos);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position, 1.0));
        out.vert.normal = hit.normal;
        out.vert.hit_index = hit_index;
    }

#ifdef LINES
    if(!all(probe_pos == coord)) {
        out.sv_position = asfloat(0xFFFFFFFF);
    }
#endif

    return out;
}


[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    const DDGIRayHit hit = in_hits[in.hit_index];

    return float4(lerp(0.1, 1.0, reinhard(hit.radiance)), 1.0);
}

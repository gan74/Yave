#include "lib/gi.slang"


[[vk::binding(0)]]
Sampler2D probes;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
Sampler3D<uint> in_grid;

[[vk::binding(3)]]
cbuffer Params {
    uint frame_id;
};



struct PixelStageIn {
    float3 normal;
    uint instance_index;
};

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    PixelStageIn vert;
};






[shader("vertex")]
VertexStageOut vert_main(StdVertexStageIn in) {
    const int3 coord = gi.compute_grid_coord(semantics.instance_index);
    const float3 position = gi.compute_world_coord(coord, camera.position);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position + in.position * 0.1, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.instance_index = semantics.instance_index;
    }

    if(in_grid[coord] != frame_id) {
        out.sv_position = asfloat(0xFFFFFFFF);
    }

    return out;
}


[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    const float2 uv = saturate(octahedron_encode(in.normal));
    const float4 irr = probes[uint2(clamp(uv * gi.probe_size, 0, gi.probe_size - 1)) + gi.compute_atlas_coord(in.instance_index) * gi.probe_size];

    return float4(identifying_color(in.instance_index), 1.0);
    // return float4(reinhard(irr.rgb), 1.0);

}

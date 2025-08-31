#include "lib/gi.slang"


[[vk::binding(0)]]
Sampler2D probes;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;


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
    const float3 position = gi.compute_world_coord(gi.compute_grid_coord(semantics.instance_index), camera.position);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position + in.position * 0.2, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.instance_index = semantics.instance_index;
    }

    return out;
}


[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    const float2 uv = saturate(octahedron_encode(in.normal));
    const float4 irr = probes[uint2(clamp(uv * gi.probe_size, 0, gi.probe_size - 1)) + gi.compute_atlas_coord(in.instance_index) * gi.probe_size];

    // return float4(identifying_color(in.instance_index), 1.0);
    return float4(reinhard(irr.rgb), 1.0);

}

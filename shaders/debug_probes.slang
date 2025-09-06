#include "lib/ddgi.slang"


[[vk::binding(0)]]
Sampler2D<float3> in_irradiance;

[[vk::binding(1)]]
Sampler2D<float2> in_distance;

[[vk::binding(2)]]
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
    const int3 coord = ddgi.compute_grid_coord(semantics.instance_index);
    const float3 position = ddgi.compute_world_coord(coord, camera.position);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position + in.position * 0.1, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.instance_index = semantics.instance_index;
    }

    return out;
}


[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    const uint2 probe_corner = ddgi.atlas_coord(in.instance_index) + 1;
    const float2 probe_coord = octahedron_encode(in.normal) * ddgi.probe_size_no_border;
    const float3 irr = in_irradiance.SampleLevel((probe_corner + probe_coord) / image_size(in_irradiance), 0.0);
    // const float dist = in_distance.SampleLevel((probe_corner + probe_coord) / image_size(in_distance), 0.0).x;

    // return float4(saturate(dist / sqrt(2.0)).xxx, 1.0);
    // return float4(identifying_color(in.instance_index), 1.0);
    return float4(reinhard(irr), 1.0);

}

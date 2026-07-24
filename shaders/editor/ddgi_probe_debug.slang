#include "../lib/utils.slang"
#include "../lib/ddgi.slang"


struct VertexStageOut {
    float4 sv_position : SV_Position;
    float3 normal;
    uint probe_index;
};


[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
StructuredBuffer<StaticMeshData> mesh_datas;

[[vk::binding(2)]]
Sampler2D<float3> in_irradiance;

[[vk::binding(3)]]
cbuffer Params {
    float probe_spacing;
    float probe_radius;
    uint mesh_data_index;
    uint _pad;
};


[shader("vertex")]
VertexStageOut vert_main() {
    const uint probe_index = uint(semantics.instance_index);
    const int3 grid_coord = ddgi.compute_grid_coord(probe_index);
    const float3 center = ddgi.probe_world_position(grid_coord, probe_spacing);

    const StdVertexData vert = read_vertex(uint(semantics.vertex_index), mesh_datas[mesh_data_index]);
    const float3 local_pos = vert.position;
    const float3 world_pos = center + local_pos * probe_radius;

    VertexStageOut out;
    out.sv_position = mul(camera.curr.view_proj, float4(world_pos, 1.0));
    out.normal = normalize(local_pos);
    out.probe_index = probe_index;
    return out;
}

[shader("fragment")]
float4 frag_main(VertexStageOut in) {
    const uint2 probe_corner = ddgi.irradiance_atlas_coord(in.probe_index);
    const float2 sample_coord = octahedron_encode(in.normal) * float(ddgi.irradiance_probe_size - 1);
    const float2 uv = (float2(probe_corner) + sample_coord + 0.5) / float2(ddgi.irradiance_atlas_size);
    const float3 irradiance = in_irradiance.SampleLevel(uv, 0.0);

    return float4(linear_to_sRGB(irradiance), 1.0);
}

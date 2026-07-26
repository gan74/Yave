#include "lib/utils.slang"
#include "lib/ddgi.slang"


struct VertexStageOut {
    float4 sv_position : SV_Position;
    float3 normal;
    uint global_probe_index;
};


[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
StructuredBuffer<StaticMeshData> mesh_datas;

[[vk::binding(2)]]
Sampler2D<float3> in_probe_tex;

[[vk::binding(3)]]
cbuffer Params {
    float base_probe_spacing;
    float spacing_scale;
    uint cascade_count;
    uint display_irradiance;
    float probe_radius;
    uint mesh_data_index;
    uint padding;
};


[shader("vertex")]
VertexStageOut vert_main() {
    const uint global_probe_index = uint(semantics.instance_index);

    uint cascade;
    uint local_probe_index;
    ddgi.decode_global_probe_index(global_probe_index, cascade, local_probe_index);

    const float probe_spacing = ddgi.cascade_probe_spacing(cascade, base_probe_spacing, spacing_scale);
    const int3 grid_coord = ddgi.compute_grid_coord(local_probe_index);
    const float3 center = ddgi.probe_world_position(grid_coord, probe_spacing);

    const StdVertexData vert = read_vertex(uint(semantics.vertex_index), mesh_datas[mesh_data_index]);
    const float3 local_pos = vert.position;
    const float3 world_pos = center + local_pos * probe_radius;

    VertexStageOut out;
    out.sv_position = mul(camera.curr.view_proj, float4(world_pos, 1.0));
    out.normal = normalize(local_pos);
    out.global_probe_index = global_probe_index;
    return out;
}

[shader("fragment")]
float4 frag_main(VertexStageOut in) {
    const uint probe_size = display_irradiance != 0 ? ddgi.irradiance_probe_size : ddgi.radiance_probe_size;
    const uint2 atlas_size = display_irradiance != 0
        ? ddgi.irradiance_atlas_size(cascade_count)
        : ddgi.radiance_atlas_size(cascade_count)
    ;
    const uint2 probe_corner = display_irradiance != 0
        ? ddgi.irradiance_atlas_coord(in.global_probe_index)
        : ddgi.radiance_atlas_coord(in.global_probe_index)
    ;

    const float2 sample_coord = octahedron_encode(in.normal) * float(probe_size - 1);
    const float2 uv = (float2(probe_corner) + sample_coord + 0.5) / float2(atlas_size);
    const float3 radiance = in_probe_tex.SampleLevel(uv, 0.0);

    return float4(linear_to_sRGB(radiance), 1.0);
}

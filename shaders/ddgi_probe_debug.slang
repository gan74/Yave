#include "lib/utils.slang"
#include "lib/ddgi.slang"


struct VertexStageOut {
    float4 sv_position : SV_Position;
    float3 normal;
    uint atlas_probe_index;
};


[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
StructuredBuffer<StaticMeshData> mesh_datas;

[[vk::binding(2)]]
Sampler2D<float3> in_probe_tex;

[[vk::binding(3)]]
Sampler2D<uint> probe_grid;

[[vk::binding(4)]]
cbuffer Params {
    float probe_spacing;
    uint atlas_probe_count;
    uint display_irradiance;
    float probe_radius;

    uint mesh_data_index;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};


[shader("vertex")]
VertexStageOut vert_main() {
    const uint grid_probe_index = uint(semantics.instance_index);
    const int3 grid_coord = ddgi.compute_grid_coord(grid_probe_index);
    const uint atlas_probe_index = ddgi.load_probe_index(probe_grid, grid_coord);

    const StdVertexData vert = read_vertex(uint(semantics.vertex_index), mesh_datas[mesh_data_index]);
    const float3 local_pos = vert.position;

    const float3 center = ddgi.probe_world_position(grid_coord, probe_spacing);
    const float3 world_pos = center + local_pos * probe_radius;

    VertexStageOut out;
    out.sv_position = mul(camera.curr.view_proj, float4(world_pos, 1.0));
    out.normal = normalize(local_pos);
    out.atlas_probe_index = atlas_probe_index;

    if(atlas_probe_index == ddgi.invalid_probe_index) {
        out.sv_position.x = asfloat(constants.max_uint);
    }

    return out;
}

[shader("fragment")]
float4 frag_main(VertexStageOut in) {
    if(in.atlas_probe_index == ddgi.invalid_probe_index) {
        discard;
    }

    const uint probe_size = display_irradiance != 0 ? ddgi.irradiance_probe_size : ddgi.radiance_probe_size;

    const uint2 atlas_size = display_irradiance != 0
        ? ddgi.irradiance_atlas_size(atlas_probe_count)
        : ddgi.radiance_atlas_size(atlas_probe_count)
    ;
    const uint2 probe_corner = display_irradiance != 0
        ? ddgi.irradiance_atlas_coord(in.atlas_probe_index)
        : ddgi.radiance_atlas_coord(in.atlas_probe_index)
    ;

    const float2 sample_coord = octahedron_encode(in.normal) * float(probe_size - 1);
    const float2 uv = (float2(probe_corner) + sample_coord + 0.5) / float2(atlas_size);

    const float3 color = in_probe_tex.SampleLevel(uv, 0.0);
    return float4(color, 1.0);
}

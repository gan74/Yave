#include "lib/utils.slang"
#include "lib/ddgi.slang"


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
Sampler2D<float3>in_radiance;

[[vk::binding(3)]]
Sampler2D<float3> in_irradiance;

[[vk::binding(4)]]
Sampler2D<float2> in_distance;

[[vk::binding(5)]]
Sampler2D<uint> probe_grid;

[[vk::binding(6)]]
cbuffer Params {
    float probe_spacing;
    uint atlas_probe_count;
    uint display_mode;
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
    const uint probe_index = ddgi.load_probe_index(probe_grid, grid_coord);

    const StdVertexData vert = read_vertex(uint(semantics.vertex_index), mesh_datas[mesh_data_index]);
    const float3 local_pos = vert.position;

    const float3 center = ddgi.probe_world_position(grid_coord, probe_spacing);
    const float3 world_pos = center + local_pos * probe_radius;

    VertexStageOut out;
    out.sv_position = mul(camera.curr.view_proj, float4(world_pos, 1.0));
    out.normal = normalize(local_pos);
    out.probe_index = probe_index;

    if(probe_index == ddgi.invalid_probe_index) {
        out.sv_position.x = asfloat(constants.max_uint);
    }

    return out;
}


float3 fetch_probe(uint probe_index, float2 probe_uvs) {
    uint probe_data_size = 0;
    uint2 atlas_size = uint2(0);
    uint2 probe_corner = uint2(0);
    switch(display_mode) {
        case 1:
        case 3: {
            probe_data_size = ddgi.radiance_probe_data_size;
            atlas_size = ddgi.radiance_atlas_size(atlas_probe_count);
            probe_corner = ddgi.radiance_atlas_coord(probe_index) + ddgi.probe_border;
        } break;
        case 2: {
            probe_data_size = ddgi.irradiance_probe_data_size;
            atlas_size = ddgi.irradiance_atlas_size(atlas_probe_count);
            probe_corner = ddgi.irradiance_atlas_coord(probe_index) + ddgi.probe_border;
        } break;
    }


    const float2 sample_coord = probe_uvs * probe_data_size;
    const float2 uv = (float2(probe_corner) + sample_coord) / float2(atlas_size);
    switch(display_mode) {
        case 1: return in_radiance.SampleLevel(uv, 0.0);
        case 2: return in_irradiance.SampleLevel(uv, 0.0);
        case 3: return in_distance.SampleLevel(uv, 0.0).xxx;
    }
    return float3(0.0);
}

[shader("fragment")]
float4 frag_main(VertexStageOut in) {
    if(in.probe_index == ddgi.invalid_probe_index) {
        discard;
    }

    const float2 probe_uv = octahedron_encode(in.normal);
    return float4(fetch_probe(in.probe_index, probe_uv), 0.0);
}

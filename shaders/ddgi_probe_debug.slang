#include "lib/utils.slang"
#include "lib/structs.slang"
#include "lib/ddgi.slang"

#ifdef COUNT

#define DEBUG
#include "lib/debug.slang"


static const int2 text_offset = int2(8, 8);
static const float2 font_size = float2(8.0, 15.0);



[[vk::binding(0)]]
StructuredBuffer<uint> inactive_probes;

[[vk::binding(1)]]
cbuffer Params {
    uint max_probe_count;
    uint padding_0;
    uint padding_1;
    uint padding_2;
};


[shader("fragment")]
float4 frag_main() {
    const int2 coord = int2(semantics.frag_coord.xy);
    const uint count = max_probe_count - inactive_probes[0];
    const float digits = max(1.0, count == 0 ? 1.0 : floor(log2(float(count)) / log2(10.0)) + 1.0);

    float glyph = 0.0;

    const uint prefix[15] = {
        CHAR_A, CHAR_C, CHAR_T, CHAR_I, CHAR_V, CHAR_E, CHAR_SPACE,
        CHAR_P, CHAR_R, CHAR_O, CHAR_B, CHAR_E, CHAR_S, CHAR_COLON, CHAR_SPACE
    };
    for(uint i = 0; i != 15; ++i) {
        glyph += debug.print_char(float2(coord), float2(text_offset) + float2(i * font_size.x, 0.0), font_size, prefix[i]);
    }

    const float2 count_offset = float2(text_offset) + float2(15.0 * font_size.x, 0.0);
    glyph += debug.print_value(float2(coord), count_offset, font_size, float(count), digits, 0.0);

    if(glyph <= 0.0) {
        discard;
    }

    return float4(1.0);
}

#else

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
StructuredBuffer<DDGIProbeData> probe_datas;

[[vk::binding(6)]]
StructuredBuffer<uint> inactive_probes;

[[vk::binding(7)]]
cbuffer Params {
    float probe_spacing;
    uint max_probe_count;
    uint display_mode;
    float probe_radius;

    uint mesh_data_index;
    uint frame_id;
    uint max_visible_age;
    uint padding_0;
};


float probe_scale(uint probe_index) {
    const uint age = frame_id - probe_datas[probe_index].last_visible;
    return saturate(1.0 - float(age) / float(max_visible_age));
}


[shader("vertex")]
VertexStageOut vert_main() {
    const uint probe_index = semantics.instance_index;

    const StdVertexData vert = read_vertex(uint(semantics.vertex_index), mesh_datas[mesh_data_index]);
    const float3 local_pos = vert.position;

    const float linear_scale = probe_scale(probe_index);
    const float scale = 1.0 - pow(1.0 - linear_scale, 5.0);

    const float3 center = probe_datas[probe_index].probe_coord * ddgi.probe_spacing_for_lod(probe_spacing, probe_datas[probe_index].lod);
    const float3 world_pos = center + local_pos * probe_radius * scale;

    VertexStageOut out;
    out.sv_position = mul(camera.curr.view_proj, float4(world_pos, 1.0));
    out.normal = normalize(local_pos);
    out.probe_index = probe_index;

    if(probe_datas[probe_index].hash_bucket == constants.max_uint) {
        out.sv_position.x = asfloat(constants.max_uint);
    }

    return out;
}


float3 fetch_probe(uint probe_index, float2 probe_uvs) {
    if(display_mode == 4) {
        return identifying_color(probe_datas[probe_index].lod);
    }

    uint probe_data_size = 0;
    uint2 atlas_size = uint2(0);
    uint2 probe_corner = uint2(0);
    switch(display_mode) {
        case 1:
        case 3: {
            probe_data_size = ddgi.radiance_probe_data_size;
            atlas_size = ddgi.radiance_atlas_size(max_probe_count);
            probe_corner = ddgi.radiance_atlas_coord(probe_index) + ddgi.probe_border;
        } break;
        case 2: {
            probe_data_size = ddgi.irradiance_probe_data_size;
            atlas_size = ddgi.irradiance_atlas_size(max_probe_count);
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
    return float3(1.0, 0.0, 0.0);
}

[shader("fragment")]
float4 frag_main(VertexStageOut in) {
    const float2 probe_uv = octahedron_encode(in.normal);
    return float4(fetch_probe(in.probe_index, probe_uv), 0.0);
}

#endif

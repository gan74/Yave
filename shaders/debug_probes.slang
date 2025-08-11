#include "lib/utils.slang"


[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
Sampler3D<uint> in_grid;

[[vk::binding(2)]]
cbuffer Params {
    uint frame_id;
};




struct PixelStageIn {
    float3 normal;
    uint probe_index;
};

struct VertexStageOut {
    float4 sv_position  : SV_Position;
    PixelStageIn vert;
};



[shader("vertex")]
VertexStageOut vert_main(StdVertexStageIn in) {
    const int3 coord = uint3(
        (semantics.instance_index / 4096) % 64,
        (semantics.instance_index / 64) % 64,
        semantics.instance_index % 64
    );

    const float3 position = int3(camera.position) + coord - int3(32);

    VertexStageOut out;
    {
        out.sv_position = mul(camera.curr.view_proj, float4(position + in.position * 0.1, 1.0));
        out.vert.normal = unpack_2_10_10_10(in.packed_normal_tangent_sign.x).xyz;
        out.vert.probe_index = hash(semantics.instance_index);
    }

    if(in_grid[coord] != frame_id) {
        out.sv_position.x = asfloat(0xFFFFFFFF);
    }

    return out;
}



[shader("fragment")]
float4 frag_main(PixelStageIn in) {
    // return float4(1, 0, 0, 1);
    return float4(heat_spectrum(float(in.probe_index % 1024) / 1024.0), 1.0);
}

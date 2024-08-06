#include "lib/utils.slang"


struct VertexStageIn {
    float3 position;
    uint packed_color_Packed;
};

struct VertexStageOut {
    float4 sv_position : SV_Position;
    float4 color;
};


[[vk::binding(0)]]
cbuffer Buffer_Inline {
    float4x4 view_proj;
};



[shader("vertex")]
VertexStageOut vert_main(VertexStageIn in) {
    VertexStageOut out;
    {
        out.sv_position = mul(view_proj, float4(in.position, 1.0));
        out.color = unpack_color(in.packed_color_Packed);
    }
    return out;
}

[shader("fragment")]
float4 frag_main(float4 color) {
    return color;
}

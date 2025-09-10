#include "lib/utils.slang"


struct VertexStageIn {
    float3 position;
    uint packed_color_Packed;
};

struct VertexStageOut {
    float4 sv_position : SV_Position;
    float3 position;
    uint color;
};


[[vk::binding(0)]]
cbuffer Params {
    float4x4 view_proj;
    float3 padding;
    uint lit;
};



[shader("vertex")]
VertexStageOut vert_main(VertexStageIn in) {
    VertexStageOut out;
    {
        out.sv_position = mul(view_proj, float4(in.position, 1.0));
        out.position = in.position;
        out.color = in.packed_color_Packed;
    }
    return out;
}

[shader("fragment")]
float4 frag_main(VertexStageOut in) {
    float4 color = unpack_color(in.color);
    if(lit != 0) {
        const float3 normal = normalize(cross(ddx(in.position), ddy(in.position)));
        const float3 sun_dir = normalize(float3(0.2, 0.3, -1.0));
        color.rgb *= 0.25 + max(0.0, dot(normal, sun_dir) * 0.75);
    }
    return color;
}

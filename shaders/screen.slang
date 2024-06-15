import "glsl";

static const float2 uvs[] = {
    float2(0.0, 0.0),
    float2(0.0, 2.0),
    float2(2.0, 0.0),
};


struct VertexStageOut {
    float4 sv_position : SV_Position;
    float2 uv;
};


[shader("vertex")]
VertexStageOut vert_main() {
    const float2 uv = uvs[gl_VertexIndex];

    VertexStageOut out;
    {
        out.sv_position = float4((uv * 2.0 - 1.0), 0.0, 1.0);
        out.uv = uv;
    }
    return out;
}


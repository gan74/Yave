#version 450




struct VertexStageIn {
    float3 position;
    float2 uv_Packed;
    float2 size_Packed;
    uint id_Packed;
};

struct GeomStageIn {
    float3 position;
    float2 uv;
    float2 size;
    uint id;
};

struct PixelStageIn {
    float2 uv;
    float4 color;
    uint id;
};

struct GeomStageOut {
    float4 sv_position : SV_Position;
    PixelStageIn vert;
};

struct PixelStageOut {
    float4 color : SV_Target0;
    uint id : SV_Target1;
};



[[vk::binding(0)]]
Sampler2D font_texture;


[[vk::binding(1)]]
cbuffer Buffer {
    mat4 view_proj;
    vec2 viewport_size;
    float billboard_size;
};







[shader("vertex")]
GeomStageIn vert_main(VertexStageIn in) {
    GeomStageIn out;
    {
        out.position = in.position;
        out.uv = in.uv_Packed;
        out.size = in.size_Packed;
        out.id = in.id_Packed;
    }
    return out;
}




static const float2 uvs[] = {
    float2(0.0, 1.0),
    float2(0.0, 0.0),
    float2(1.0, 1.0),
    float2(1.0, 0.0),
};


[shader("geometry")]
[maxvertexcount(4)]
void geom_main(point GeomStageIn in[1], inout TriangleStream<GeomStageOut> out_stream) {
    const float4 position = mul(view_proj, float4(in[0].position, 1.0));
    const float2 size = (billboard_size * position.w) / viewport_size;

    for(uint i = 0; i != 4; ++i) {
        GeomStageOut out;
        {
            out.sv_position = float4(position.xy + (uvs[i] - 0.5) * size, position.zw);
            out.vert.uv = in[0].uv + uvs[i] * in[0].size;
            out.vert.id = in[0].id;
            out.vert.color = float4(1.0);
        }
        out_stream.Append(out);
    }
}


[shader("fragment")]
PixelStageOut frag_main(PixelStageIn in) {
    const float smoothing = 1.0 / 16.0;
    const float alpha_edge = 0.7;

    const float4 color = in.color * font_texture.Sample(in.uv);
    const float alpha_step = smoothstep(alpha_edge - smoothing, alpha_edge + smoothing, color.a);

    if(alpha_step < 0.1) {
        discard;
    }

    PixelStageOut out;
    {
        out.color = vec4(color.rgb, alpha_step);
        out.id = in.id + 1;
    }
    return out;
}


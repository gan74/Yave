#include "../lib/utils.slang"




struct VertexStageIn {
    float2 position;
    float2 uv_Packed;
    uint packed_color_Packed;
};

struct PixelStageIn {
    float2 uv;
    float4 color;
};


struct VertexStageOut {
    float4 sv_position : SV_Position;
    PixelStageIn vert;
};





[[vk::binding(0)]]
Sampler2D font_texture;

[[vk::binding(1)]]
cbuffer Buffer {
    float2 viewport_size;
    float2 viewport_offset;
};


float sampling_factor(Sampler2D tex, float2 uv) {
    const float2 size = float2(sampler_size(tex).xy);
    const float2 coord = uv * size;

    const float x_factor = max(abs(ddx(coord).x), abs(ddx(coord).y));
    const float y_factor = max(abs(ddy(coord).x), abs(ddy(coord).y));

    return max(x_factor, y_factor);
}

float4 texture_min_filter(Sampler2D tex, float2 uv) {
    const float2 size = float2(sampler_size(tex).xy);
    const float2 inv_size = 1.0 / size;

    return (
       tex.Sample(uv + inv_size * float2( 1.0,  1.0)) +
       tex.Sample(uv + inv_size * float2( 1.0, -1.0)) +
       tex.Sample(uv + inv_size * float2( 1.0, -1.0)) +
       tex.Sample(uv + inv_size * float2(-1.0, -1.0))
    ) * 0.25;
}







[shader("vertex")]
VertexStageOut vert_main(VertexStageIn in) {
    const float4x4 proj = float4x4(
        2.0 / viewport_size.x, 0.0, 0.0, -1.0,
        0.0, 2.0 / -viewport_size.y, 0.0, 1.0,
        0.0, 0.0, -1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
    );

    float2 screen_pos = in.position - viewport_offset;
    screen_pos.y = viewport_size.y - screen_pos.y;

    VertexStageOut out;
    {
        out.sv_position = mul(proj, float4(screen_pos, 0.0, 1.0));
        out.vert.uv = in.uv_Packed;
        out.vert.color = unpack_color(in.packed_color_Packed);
    }
    return out;
}



[shader("fragment")]
float4 frag_main(PixelStageIn in) : SV_Target {
    float4 color = in.color;

#if 1
    if(sampling_factor(font_texture, in.uv) > 2.0) {
        color *= texture_min_filter(font_texture, in.uv);
    } else {
        color *= font_texture.Sample(in.uv);
    }
#else
    color *= font_texture.Sample(in._uv);
#endif

    // We can't do linear to sRGB conversion here, since this shader is used to render
    // images that are already in sRGB in the UI
    return color;
}


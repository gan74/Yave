import "glsl";

#include "lib/utils.slang"

// https://64.github.io/tonemapping/
// https://mynameismjp.wordpress.com/2010/04/30/a-closer-look-at-tone-mapping/
// https://www.academia.edu/24772316/Programming_Vertex_Geometry_and_Pixel_Shaders_Screenshots_of_Alan_Wake_courtesy_of_Remedy_Entertainment
// https://imdoingitwrong.wordpress.com/2010/08/19/why-reinhard-desaturates-my-blacks-3/
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ToneMapping.hlsl

[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
ConstantBuffer<ExposureParams> params;

[[vk::binding(2)]]
cbuffer Settings_Inline {
    float exposure;
    uint tone_mapper;
};

/*enum class ToneMapper {
    ACES,
    Uncharted2
    Reinhard,
    None
};*/


float3 tone_map(float3 hdr, float exposure, uint mode) {
    const float white = 10000.0;

    // Is it better to just tonemap the luminance or R, G and B separately?
    hdr = expose_RGB(hdr, exposure);

    if(mode == 0) {
        hdr = ACES(hdr) / ACES(white);
    } else if(mode == 1) {
        hdr = uncharted2(hdr) / uncharted2(white);
    } else if(mode == 2) {
        hdr = reinhard(hdr);
    }

    return hdr;
}


[shader("fragment")]
float4 frag_main() {
    const float3 hdr = in_color[int2(gl_FragCoord.xy)].rgb;

    const float3 tone_mapped = tone_map(hdr, params.exposure * exposure, tone_mapper);
    return float4(tone_mapped, 1.0);
}


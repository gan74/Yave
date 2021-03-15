#version 450

#include "lib/hdr.glsl"

// https://64.github.io/tonemapping/
// https://mynameismjp.wordpress.com/2010/04/30/a-closer-look-at-tone-mapping/
// https://www.academia.edu/24772316/Programming_Vertex_Geometry_and_Pixel_Shaders_Screenshots_of_Alan_Wake_courtesy_of_Remedy_Entertainment
// https://imdoingitwrong.wordpress.com/2010/08/19/why-reinhard-desaturates-my-blacks-3/
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ToneMapping.hlsl

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(set = 0, binding = 1) uniform Params {
    ToneMappingParams params;
};

layout(set = 0, binding = 2) uniform Settings_Inline {
    float exposure;
    uint tone_mapper;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


/*
enum class ToneMapper {
    ACES,
    Uncharted2
    Reinhard,
    None
};
*/
vec3 tone_map(vec3 hdr, float exposure, uint mode) {
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


void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const vec3 hdr = texelFetch(in_color, coord, 0).rgb;

    const vec3 tone_mapped = tone_map(hdr, params.exposure * exposure, tone_mapper);
    out_color = vec4(linear_to_sRGB(tone_mapped), 1.0);
}


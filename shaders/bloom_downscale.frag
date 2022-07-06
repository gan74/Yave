#version 450

#include "lib/utils.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform BloomParams_Inline {
    uint pass;
};

layout(location = 0) in vec2 in_uv;

vec3 karis(vec3 hdr) {
    const float avg = 1.0f / (1.0f + (luminance(hdr) * 0.25));
    return hdr * avg;
}

void main() {
    const vec2 texel_size = 1.0 / textureSize(in_color, 0);
    const float x = texel_size.x;
    const float y = texel_size.y;

    const vec3 a = texture(in_color, in_uv + vec2(-2.0 * x, 2.0 * y)).rgb;
    const vec3 b = texture(in_color, in_uv + vec2(     0.0, 2.0 * y)).rgb;
    const vec3 c = texture(in_color, in_uv + vec2( 2.0 * x, 2.0 * y)).rgb;
    const vec3 d = texture(in_color, in_uv + vec2(-2.0 * x,     0.0)).rgb;
    const vec3 e = texture(in_color, in_uv).rgb;
    const vec3 f = texture(in_color, in_uv + vec2( 2.0 * x,      0.0)).rgb;
    const vec3 g = texture(in_color, in_uv + vec2(-2.0 * x, -2.0 * y)).rgb;
    const vec3 h = texture(in_color, in_uv + vec2(     0.0, -2.0 * y)).rgb;
    const vec3 i = texture(in_color, in_uv + vec2( 2.0 * x, -2.0 * y)).rgb;
    const vec3 j = texture(in_color, in_uv + vec2(-x,  y)).rgb;
    const vec3 k = texture(in_color, in_uv + vec2( x,  y)).rgb;
    const vec3 l = texture(in_color, in_uv + vec2(-x, -y)).rgb;
    const vec3 m = texture(in_color, in_uv + vec2( x, -y)).rgb;

    out_color.a = 1.0;
    if(pass == 0) {
        const vec3 g0 = (a + b + d + e) * 0.125 * 0.25;
        const vec3 g1 = (b + c + e + f) * 0.125 * 0.25;
        const vec3 g2 = (d + e + g + h) * 0.125 * 0.25;
        const vec3 g3 = (e + f + h + i) * 0.125 * 0.25;
        const vec3 g4 = (j + k + l + m) * 0.5 * 0.25;
        out_color.rgb = karis(g0) + karis(g1) + karis(g2) + karis(g3) + karis(g4);
    } else {
        out_color.rgb =
            (e * 0.125) +
            (a + c + g + i) * 0.03125 +
            (b + d + f + h) * 0.0625 +
            (j + k + l + m) * 0.125;
    }

}


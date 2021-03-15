#version 450

#include "lib/hdr.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform BloomParams_Inline {
    float power;
    float threshold;
    float rev_threshold;
};

layout(location = 0) in vec2 in_uv;

void main() {
    const vec3 hdr = texture(in_color, in_uv).rgb;
    const float hdr_lum = luminance(hdr);

    const float hdr_weight = 1.0 / (1.0 + hdr_lum);
    const float lum = hdr_lum * hdr_weight;

    const float thresholded = max(0.0, (lum - threshold) * rev_threshold);
    const float bloom_factor = pow(thresholded, power);

    out_color = vec4(hdr * bloom_factor * hdr_weight, 1.0);
}


#version 450

#include "lib/utils.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_bloom;
layout(set = 0, binding = 1) uniform sampler2D in_color;

layout(set = 0, binding = 2) uniform BloomParams_Inline {
    float power;
    float threshold;
    float rev_threshold;
    float intensity;
};

layout(location = 0) in vec2 in_uv;

void main() {
    const vec3 hdr = texture(in_color, in_uv).rgb;
    const vec3 thresholded = bloom_threshold(hdr, power, threshold, rev_threshold);

    vec3 bloomed = texture(in_bloom, in_uv).rgb;
    bloomed += hdr - thresholded;

    out_color = vec4(mix(hdr, bloomed, intensity), 1.0);
}


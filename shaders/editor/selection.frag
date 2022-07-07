#version 450

#include "../lib/utils.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_seletion_depth;
layout(set = 0, binding = 2) uniform usampler2D in_seletion_id;

layout(location = 0) in vec2 in_uv;

const vec3 selection_color = vec3(241.0, 153.0, 41.0) / 255.0;
const int size = 2;

void main() {
    out_color = vec4(selection_color, 0.0);

    const ivec2 coords = ivec2(gl_FragCoord.xy);
    const uint base_id = texelFetch(in_seletion_id, coords, 0).r;

    float min_depth = 1.0;
    float sum = 0.0;
    float total = 0.0;
    for(int i = -size; i <= size; ++i) {
        for(int j = -size; j <= size; ++j) {
            const uint id = texelFetch(in_seletion_id, coords + ivec2(i, j), 0).r;
            const float w = dot(vec2(i, j), vec2(i, j));
            if(id != base_id) {
                min_depth = min(min_depth, texelFetch(in_seletion_depth, coords + ivec2(i, j), 0).r);
                sum += w;
            }
            total += w;
        }
    }

    if(sum > 0.0) {
        out_color.a = saturate(sum / total * 4.0);
        if(texelFetch(in_depth, coords, 0).r > min_depth) {
            out_color.a *= 0.5;
        }
    }
}


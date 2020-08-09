#version 450

#include "lib/screen_space.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform usampler2D in_ids;
layout(set = 0, binding = 1) uniform Params {
    uint selected;
};

layout(location = 0) in vec2 in_uv;

const vec3 selection_color = vec3(241.0, 153.0, 41.0) / 255.0;

void main() {
    const ivec2 coords = ivec2(gl_FragCoord.xy);
    if(texelFetch(in_ids, coords, 0).r == selected) {
        out_color = vec4(0.0);
        return;
    }

    const int size = 2;

    float sum = 0.0;
    float total = 0.0;
    for(int i = -size; i <= size; ++i) {
        for(int j = -size; j <= size; ++j) {
            const float w = dot(vec2(i, j), vec2(i, j));
            const bool s = texelFetch(in_ids, coords + ivec2(i, j), 0).r == selected;
            sum += s ? w : 0.0;
            total += w;
        }
    }

    out_color = vec4(selection_color, sum / (total * 0.25));
}


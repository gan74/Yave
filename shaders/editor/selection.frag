#version 450

#include "../lib/screen_space.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform usampler2D in_ids;
layout(set = 0, binding = 1) uniform Params_Inline {
    uint selected;
};

layout(location = 0) in vec2 in_uv;

const vec3 selection_color = vec3(241.0, 153.0, 41.0) / 255.0;

bool matches(ivec2 coords) {
    const uint value = texelFetch(in_ids, coords, 0).r;
    return value == (selected + 1);
}

float dist_to_bound(ivec2 coords, ivec2 size) {
    const vec2 dist = size - coords;
    return min(min(dist.x, dist.y), min(coords.x, coords.y) + 1);
}

void main() {
    const int size = 2;
    const ivec2 coords = ivec2(gl_FragCoord.xy);

    if(matches(coords)) {
        if(dist_to_bound(coords, textureSize(in_ids, 0).xy) <= size) {
            out_color = vec4(selection_color, 1.0);
        } else {
            out_color = vec4(0.0);
        }
        return;
    }

    float sum = 0.0;
    float total = 0.0;
    for(int i = -size; i <= size; ++i) {
        for(int j = -size; j <= size; ++j) {
            const float w = dot(vec2(i, j), vec2(i, j));
            const bool s = matches(coords + ivec2(i, j));
            sum += s ? w : 0.0;
            total += w;
        }
    }

    out_color = vec4(selection_color, sum / (total * 0.25));
}


#version 450

layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform sampler2D in_prev;

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const vec3 color = texelFetch(in_color, coord, 0).rgb;
    const vec3 prev = texelFetch(in_prev, coord, 0).rgb;

    out_color = vec4(mix(color, prev, 0.9), 1.0);
}


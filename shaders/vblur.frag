#version 450

#include "lib/blur.glsl"

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

void main() {
    const vec2 offset = vertical_blur_offset(in_color, 1.0);
    out_color = blur(in_color, in_uv, offset);
}


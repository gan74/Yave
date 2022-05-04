#version 450

#include "../lib/utils.glsl"

layout(location = 0) in vec3 in_position;
layout(location = 1) in uint in_packed_color_Packed;

layout(set = 0, binding = 0) uniform Buffer_Inline {
    mat4 view_proj;
};

layout(location = 0) out vec4 out_color;

void main() {
    out_color = unpack_color(in_packed_color_Packed);
    gl_Position = view_proj * vec4(in_position, 1.0);
}


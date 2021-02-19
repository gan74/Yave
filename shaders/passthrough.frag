#version 450

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

void main() {
    const vec4 color = texture(in_color, in_uv);
    out_color = color;
}


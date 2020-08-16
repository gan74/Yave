#version 450

layout(location = 0) in vec3 in_position;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 0, binding = 1) uniform Buffer {
    mat4 view_proj;
    vec2 viewport_size;
    float billboard_size;
};

void main() {
    gl_Position = view_proj * vec4(in_position, 1.0);
}


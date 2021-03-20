#version 450

#include "../lib/utils.glsl"

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in uint in_packed_color;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 0, binding = 1) uniform Buffer {
    vec2 viewport_size;
    vec2 viewport_offset;
};

void main() {
    const mat4 proj = mat4(2.0 / viewport_size.x, 0.0, 0.0, 0.0,
                           0.0, 2.0 / -viewport_size.y, 0.0, 0.0,
                           0.0, 0.0, -1.0, 0.0,
                           -1.0, 1.0, 0.0, 1.0);
    out_uv = in_uv;
    out_color = unpack_color(in_packed_color);

    vec2 screen_pos = in_position - viewport_offset;
    screen_pos.y = viewport_size.y - screen_pos.y;

    const vec4 pos = proj * vec4(screen_pos, 0.0, 1.0);
    gl_Position = pos;
}


#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 0, binding = 1) uniform Buffer {
    vec2 viewport_size;
};


void main() {
    const vec4 color = v_color * texture(font_texture, v_uv);

    // We can't do linear to sRGB conversion here, since this shader is used to render
    // images that are already in sRGB in the UI
    out_color = color;
}


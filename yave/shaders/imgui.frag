#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;

void main() {
	out_color = v_color * texture(font_texture, v_uv);
}

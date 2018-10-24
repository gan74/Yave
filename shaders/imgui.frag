#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 1, binding = 0) uniform Data {
	vec2 viewport_size;
} constants;

//const float gamma = 2.2f;

void main() {
	vec4 color = v_color * texture(font_texture, v_uv);
	out_color = color;
}

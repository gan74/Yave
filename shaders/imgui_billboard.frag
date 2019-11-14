#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in flat uint in_instance_id;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec4 in_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 0, binding = 1) uniform Data {
	vec2 viewport_size;
} constants;

const float smoothing = 1.0 / 16.0;
const float alpha_edge = 0.7;

void main() {
	const vec4 color = in_color * texture(font_texture, in_uv);
	const float alpha_step = smoothstep(alpha_edge - smoothing, alpha_edge + smoothing, color.a);
	out_color = vec4(color.rgb, alpha_step);
}

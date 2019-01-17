#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in uint col;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 0, binding = 1) uniform Data {
	vec2 viewport_size;
} constants;

void main() {
	vec2 viewport = constants.viewport_size;

	mat4 proj = mat4(2.0 / viewport.x, 0.0, 0.0, 0.0,
					 0.0, 2.0 / -viewport.y, 0.0, 0.0,
					 0.0, 0.0, -1.0, 0.0,
					 -1.0, 1.0, 0.0, 1.0);
	v_uv = uv;
	v_color = vec4((col >> 0) & 0xFF, (col >> 8) & 0xFF, (col >> 16) & 0xFF, (col >> 24) & 0xFF) / 255.0;
	vec4 pos = proj * vec4(position.x, viewport.y - position.y, 0.0, 1.0);
	gl_Position = pos;
}

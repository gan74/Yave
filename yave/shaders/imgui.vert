#version 450


layout(location = 0) in vec2 position;
layout(location = 1) in vec2 uv;
layout(location = 2) in uint col;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;


void main() {
	mat4 proj = mat4(2.0 / 1280.0, 0.0, 0.0, 0.0,
					 0.0, 2.0 / -768.0, 0.0, 0.0,
					 0.0, 0.0, -1.0, 0.0,
					 -1.0, 1.0, 0.0, 1.0);
	v_uv = uv;
	v_color = vec4((col >> 0) & 0xFF, (col >> 8) & 0xFF, (col >> 16) & 0xFF, (col >> 24) & 0xFF) / 255.0;
	vec4 pos = proj * vec4(position.x, 768.0-position.y, 0.0, 1.0);
	gl_Position = pos;
}

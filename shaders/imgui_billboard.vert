#version 450

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec2 in_size;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec2 v_uv;
layout(location = 2) out vec2 v_size;


void main() {
	v_position = in_position;
	v_uv = in_uv;
	v_size = in_size;
}

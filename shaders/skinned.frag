#version 450

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;

layout(location = 2) in vec3 bone_color;

void main() {
	out_color = vec4(bone_color, 1.0);
	out_normal = vec4(v_normal * 0.5 + vec3(0.5), 1.0);
}

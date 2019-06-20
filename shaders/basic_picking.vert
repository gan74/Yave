#version 450

layout(set = 0, binding = 0) uniform ViewProj {
	mat4 matrix;
} view_proj;

layout(location = 0) in vec3 in_position;
layout(location = 8) in mat4 in_model;

layout(location = 0) out uint instance_id;

void main() {
	mat3 model = mat3(in_model);
	gl_Position = view_proj.matrix * in_model * vec4(in_position, 1.0);

	instance_id = gl_InstanceIndex;
}

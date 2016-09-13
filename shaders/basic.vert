#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(binding = 0) uniform Matrices {
	mat4 model;
	mat4 view;
	mat4 proj;
} matrices;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_uv;

void main() {

	v_uv = in_uv;
	//v_normal = mat3(matrices.model) * in_normal;
	gl_Position = matrices.proj * matrices.view * matrices.model * vec4(in_position, 1.0);
}

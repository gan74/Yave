#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform ViewProj {
	mat4 view;
	mat4 proj;
} view_proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;
layout(location = 8) in mat4 in_model;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_uv;

void main() {
	v_uv = in_uv;
	v_normal = mat3(in_model) * in_normal;
	gl_Position = view_proj.proj * view_proj.view * in_model * vec4(in_position, 1.0);
}

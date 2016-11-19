#version 450
#extension GL_ARB_separate_shader_objects : enable

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform Model {
	mat4 model;
} model;

layout(set = 1, binding = 0) uniform ViewProj {
	mat4 view;
	mat4 proj;
} view_proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec2 v_uv;

void main() {
	vec4 p = model.model * vec4(in_position, 1.0);
	gl_Position = view_proj.proj * view_proj.view * p;
	
	v_position = in_position;
	v_uv = in_uv;
}
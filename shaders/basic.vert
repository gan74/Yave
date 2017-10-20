#version 450

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform ViewProj {
	mat4 matrix;
} view_proj;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;
layout(location = 8) in mat4 in_model;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec3 v_tangent;
layout(location = 2) out vec3 v_bitangent;
layout(location = 3) out vec2 v_uv;

void main() {
	v_uv = in_uv;

	mat3 model = mat3(in_model);
	v_normal = model * in_normal;
	v_tangent = model * in_tangent;
	v_bitangent = cross(v_tangent, v_normal);

	gl_Position = view_proj.matrix * in_model * vec4(in_position, 1.0);
}

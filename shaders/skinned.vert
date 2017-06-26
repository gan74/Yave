#version 450

#include "yave.glsl"

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

layout(location = 4) in uvec4 in_skin_indexes;
layout(location = 5) in vec4 in_skin_weights;

layout(location = 8) in mat4 in_model;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_uv;

layout(location = 2) out vec3 bone_color;

void main() {

	vec4 indexes = in_skin_indexes / 64.0;
	bone_color = spectrum(indexes.x) * in_skin_weights.x +
				 spectrum(indexes.y) * in_skin_weights.y +
				 spectrum(indexes.z) * in_skin_weights.z +
				 spectrum(indexes.w) * in_skin_weights.w;

	v_uv = in_uv;
	v_normal = mat3(in_model) * in_normal;
	gl_Position = view_proj.matrix * in_model * vec4(in_position, 1.0);
}

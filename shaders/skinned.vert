#version 450

#include "yave.glsl"

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform ViewProj {
	mat4 matrix;
} view_proj;

layout(set = 1, binding = 0) uniform Bones {
	mat4 transforms[max_bones];
} bones;

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
	bone_color = spectrum(in_skin_indexes.x) * in_skin_weights.x +
				 spectrum(in_skin_indexes.y) * in_skin_weights.y +
				 spectrum(in_skin_indexes.z) * in_skin_weights.z +
				 spectrum(in_skin_indexes.w) * in_skin_weights.w;


	mat4 bone_matrix = in_skin_weights.x * bones.transforms[in_skin_indexes.x] +
					   in_skin_weights.y * bones.transforms[in_skin_indexes.y] +
					   in_skin_weights.z * bones.transforms[in_skin_indexes.z] +
					   in_skin_weights.w * bones.transforms[in_skin_indexes.w];

	v_uv = in_uv;
	v_normal = mat3(in_model) * mat3(bone_matrix) * in_normal;
	gl_Position = view_proj.matrix * in_model * bone_matrix * vec4(in_position, 1.0);
}

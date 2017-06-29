#version 450

#include "yave.glsl"

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform ViewProj {
	mat4 matrix;
} view_proj;

layout(set = 1, binding = 0) uniform Bones {
	mat4 transforms[max_bones * 2];
} bones;


layout(location = 8) in mat4 in_model;

layout(location = 2) out vec3 bone_color;


void main() {
	uint bone_id = gl_VertexIndex / 2;
	bone_color = spectrum(bone_id);

	gl_Position = view_proj.matrix * in_model * bones.transforms[gl_VertexIndex] * vec4(0.0, 0.0, 0.0, 1.0);
}

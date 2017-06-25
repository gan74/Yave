#version 450

out gl_PerVertex {
	vec4 gl_Position;
};

layout(set = 0, binding = 0) uniform ViewProj {
	mat4 matrix;
} view_proj;

layout(set = 1, binding = 0) uniform sampler2D in_height;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec2 v_uv;

void main() {
	v_uv = in_uv;
	v_normal = in_normal;

	float height = texture(in_height, in_uv).x;
	// recompute normal
	gl_Position = view_proj.matrix * vec4(in_position.xy, height, 1.0);
}

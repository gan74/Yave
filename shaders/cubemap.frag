#version 450

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(set = 1, binding = 0) uniform samplerCube in_cubemap;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;


void main() {
	vec4 tex_color = texture(in_cubemap, v_normal);

	out_color = tex_color;
	out_normal = vec4(v_normal * 0.5 + vec3(0.5), 1.0);
}

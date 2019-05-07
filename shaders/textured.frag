#version 450

#include "yave.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(set = 1, binding = 0) uniform sampler2D in_color;
layout(set = 1, binding = 1) uniform sampler2D in_normal;
layout(set = 1, binding = 2) uniform sampler2D in_roughness_metallic;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_tangent;
layout(location = 2) in vec3 v_bitangent;
layout(location = 3) in vec2 v_uv;

vec3 unpack_normal_map(vec2 normal) {
	normal = normal * 2.0 - vec2(1.0);
	return vec3(normal, 1.0 - sqrt(dot(normal, normal)));
}

void main() {
	vec3 color = texture(in_color, v_uv).rgb;
	vec2 roughness_metallic = texture(in_roughness_metallic, v_uv).xy;
	vec3 normal = unpack_normal_map(texture(in_normal, v_uv).xy);

	vec3 mapped_normal = normal.x * v_tangent +
						 normal.y * v_bitangent +
						 normal.z * v_normal;

	out_color = pack_color(color, roughness_metallic.y);
	out_normal = pack_normal(mapped_normal, roughness_metallic.x);
}

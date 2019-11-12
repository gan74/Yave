#version 450

#include "yave.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(set = 1, binding = 0) uniform sampler2D in_color;
layout(set = 1, binding = 1) uniform sampler2D in_normal_map;
layout(set = 1, binding = 2) uniform sampler2D in_roughness;
layout(set = 1, binding = 3) uniform sampler2D in_metallic;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_tangent;
layout(location = 2) in vec3 in_bitangent;
layout(location = 3) in vec2 in_uv;

vec3 unpack_normal_map(vec2 normal) {
	normal = normal * 2.0 - vec2(1.0);
	return vec3(normal, 1.0 - sqrt(dot(normal, normal)));
}

void main() {
	vec3 color = texture(in_color, in_uv).rgb;
	vec3 normal = unpack_normal_map(texture(in_normal_map, in_uv).xy);

	float roughness = texture(in_roughness, in_uv).x;
	float metallic = texture(in_metallic, in_uv).x;

	vec3 mapped_normal = normal.x * in_tangent +
						 normal.y * in_bitangent +
						 normal.z * in_normal;

	out_color = pack_color(color, metallic);
	out_normal = pack_normal(mapped_normal, roughness);
}

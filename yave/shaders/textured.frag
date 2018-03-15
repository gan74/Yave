#version 450

#include "yave.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(set = 1, binding = 0) uniform sampler2D in_color;
layout(set = 1, binding = 1) uniform sampler2D in_roughness;
layout(set = 1, binding = 2) uniform sampler2D in_metallic;
layout(set = 1, binding = 3) uniform sampler2D in_normal;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_tangent;
layout(location = 2) in vec3 v_bitangent;
layout(location = 3) in vec2 v_uv;


void main() {
	vec3 color = texture(in_color, v_uv).rgb;
	float roughness = texture(in_roughness, v_uv).x;
	float metallic = texture(in_metallic, v_uv).x;
	vec3 normal = normalize(texture(in_normal, v_uv).xyz * 2.0 - vec3(1.0));

	vec3 mapped_normal = normal.x * v_tangent +
						 normal.y * v_bitangent +
						 normal.z * v_normal;

	out_color = pack_color(color, metallic);
	out_normal = pack_normal(mapped_normal, roughness);
}

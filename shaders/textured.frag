#version 450

#include "yave.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(set = 1, binding = 0) uniform sampler2D in_color;
layout(set = 1, binding = 1) uniform sampler2D in_roughness;
layout(set = 1, binding = 2) uniform sampler2D in_metallic;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;


void main() {
	vec3 color = texture(in_color, v_uv).rgb;
	float roughness = texture(in_roughness, v_uv).x;
	float metallic = texture(in_metallic, v_uv).x;

	out_color = pack_color(color, metallic);
	out_normal = pack_normal(v_normal, roughness);
}

#version 450

#include "yave.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_tangent;
layout(location = 2) in vec3 in_bitangent;
layout(location = 3) in vec2 in_uv;

void main() {
	out_color = pack_color(vec3(0.5), 0.0);
	out_normal = pack_normal(in_normal, 0.2);
}

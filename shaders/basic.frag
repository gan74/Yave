#version 450

#include "yave.glsl"

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 out_normal;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_tangent;
layout(location = 2) in vec3 v_bitangent;
layout(location = 3) in vec2 v_uv;

void main() {
	out_color = pack_color(vec3(1.0), 0.0);
	out_normal = pack_normal(v_normal, 0.2);
}

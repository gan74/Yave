#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(set = 2, binding = 0) uniform sampler2D in_color;
layout(set = 2, binding = 1) uniform sampler2D in_depth;

float rand(vec2 uv) {
	return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 rand2(vec2 uv) {
	float angle = rand(uv) * 6.28318530718;
	return vec2(cos(angle), sin(angle));
}

float blur_amount(vec2 uv) {
	vec4 d = textureGather(in_depth, v_uv, 0);
	float min = min(min(d.x, d.y), min(d.z, d.w));
	float max = max(max(d.x, d.y), max(d.z, d.w));
	
	return 2.0 * (max - min) / (max + min);
}


void main() {
	out_color = vec4(blur_amount(v_uv));
	
}

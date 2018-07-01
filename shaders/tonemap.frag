#version 450

#include "yave.glsl"

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 out_color;

const float gamma = 2.0;
const float inv_gamma = 1.0 / gamma;


void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);
	vec3 hdr = texelFetch(in_color, coord, 0).rgb;

	vec3 ldr = uncharted2(hdr);

	out_color = vec4(pow(ldr, vec3(inv_gamma)), 1.0);
}



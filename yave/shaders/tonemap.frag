#version 450

#include "yave.glsl"

layout(set = 0, binding = 0) uniform sampler2D in_color;

layrout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 out_color;

const float gamma = 2.2;

void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy);
	vec4 inv_gamma = vec4(1.0 / gamma);

	vec4 color = texelFetch(in_color, coord, 0);

	out_color = pow(color, inv_gamma);
}



#version 450

layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform Data {
	ivec2 view_size;
	ivec2 view_offset;
	ivec2 render_size;
} constants;

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 out_color;


void main() {
	ivec2 coord = ivec2(gl_FragCoord.xy) - constants.view_offset;
	ivec2 offset = (constants.render_size - constants.view_size) / 2;

	vec4 color = texelFetch(in_color, coord + offset, 0);

	out_color = color;
}



#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(set = 2, binding = 0) uniform sampler2D in_texture;


void main() {
	out_color = texture(in_texture, v_uv).bgra;
}

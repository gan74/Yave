#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(set = 2, binding = 0) uniform sampler2D in_color;
layout(set = 2, binding = 1) uniform sampler2D in_normal;
layout(set = 2, binding = 2) uniform sampler2D in_depth;

float depth_remap(float d) {
	float start = 0.6;
	return (d - start) / start;
}

void main() {
	out_color = texture(in_color, v_uv);
	//out_color = vec4(depth_remap(texture(in_depth, v_uv).r));
}

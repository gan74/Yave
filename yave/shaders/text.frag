#version 450

layout(location = 0) out vec4 out_color;

layout(set = 1, binding = 0) uniform sampler2D in_texture;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;


void main() {
	const float threshold = 0.5;
	float sdf = texture(in_texture, v_uv).r;

	if(sdf < threshold) {
		discard;
	}

	out_color = vec4(1.0);
	//out_color = vec4(sdf > threshold ? sdf : 0.0, sdf < threshold ? sdf : 0.0, 0, 0);
}

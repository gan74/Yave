#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;
layout(set = 1, binding = 0) uniform sampler2D in_texture;

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec2 v_uv;
//layout(location = 2) in vec3 v_barycentric;

float saturate(float x) {
	return clamp(x, 0.0, 1.0);
}

float half_lambert(vec3 normal, vec3 light_direction) {
	float lambert = saturate(dot(normalize(v_normal), light_direction));
	return 0.25 + lambert * 0.75;
}

/*float edge_factor() {
	float width = 0.5;
	vec3 a3 = smoothstep(vec3(0.0), fwidth(v_barycentric) * width, v_barycentric);
	return min(min(a3.x, a3.y), a3.z);
}*/

void main() {
	vec4 wire_color = vec4(0);
	vec4 tex_color = texture(in_texture, v_uv);

	//out_color = mix(wire_color, tex_color, edge_factor());
	out_color = tex_color;
}

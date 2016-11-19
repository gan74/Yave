#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(set = 2, binding = 0) uniform sampler2D in_color;
layout(set = 2, binding = 1) uniform sampler2D in_normal;
layout(set = 2, binding = 2) uniform sampler2D in_depth;


vec3 normal_from_depth(float depth, vec2 uv) {
	ivec2 size = textureSize(in_depth, 0);
	float d_x = textureLodOffset(in_depth, uv, 0, ivec2(1, 0)).r;
	float d_y = textureLodOffset(in_depth, uv, 0, ivec2(0, 1)).r;
	
	vec3 v_x = vec3(1.0 / size.x, 0.0, d_x - depth);
	vec3 v_y = vec3(0.0, 1.0 / size.y, d_y - depth);
	
	return normalize(cross(v_x, v_y));
}

float saturate(float x) {
	return min(1.0, max(0.0, x));
}

void main() {
	/*float depth = textureLod(in_depth, v_uv, 0).r;
	vec3 normal = normal_from_depth(depth, v_uv);*/

	vec3 normal = texture(in_normal, v_uv).xyz * 2.0 - vec3(1.0);
	
	vec3 light_dir = normalize(vec3(1, 1, 1));
	
	float lambert = saturate(dot(normal, light_dir)) * 0.75 + 0.25;
	
	out_color = textureLod(in_color, v_uv, 0) * lambert;
}

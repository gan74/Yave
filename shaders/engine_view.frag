#version 450

#include "lib/utils.glsl"
#include "lib/gbuffer.glsl"


layout(set = 0, binding = 0) uniform Target {
	uint target_index;
};

layout(set = 0, binding = 1) uniform sampler2D in_final;
layout(set = 0, binding = 2) uniform sampler2D in_depth;
layout(set = 0, binding = 3) uniform sampler2D in_color;
layout(set = 0, binding = 4) uniform sampler2D in_normal;


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
	const ivec2 coord = ivec2(gl_FragCoord.xy);

	vec3 albedo;
	float metallic;
	vec3 normal;
	float roughness;
	unpack_color(texelFetch(in_color, coord, 0), albedo, metallic);
	unpack_normal(texelFetch(in_normal, coord, 0), normal, roughness);

	const float depth = texelFetch(in_depth, coord, 0).r;
	const vec3 final = texelFetch(in_final, coord, 0).rgb;

	vec3 color = final;

	if(target_index == 1) {
		color = albedo;
	} else if(target_index == 2) {
		color = normal * 0.5 + 0.5;
	} else if(target_index == 3) {
		color = vec3(metallic);
	} else if(target_index == 4) {
		color = vec3(roughness);
	} else if(target_index == 5) {
		color = vec3(pow(depth, 0.35));
	}

	out_color = vec4(color, 1.0);
}



#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL

vec4 pack_color(vec3 color, float metallic) {
	return vec4(color, metallic);
}

vec4 pack_normal(vec3 normal, float roughness) {
	return vec4(normalize(normal) * 0.5 + vec3(0.5), roughness);
}

void unpack_color(vec4 buff, out vec3 color, out float metallic) {
	color = buff.rgb;
	metallic = buff.a;
}

void unpack_normal(vec4 buff, out vec3 normal, out float roughness) {
	normal = normalize(buff.xyz * 2.0 - vec3(1.0));
	roughness = max(0.05, buff.w);
}

#endif

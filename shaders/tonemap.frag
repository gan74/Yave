#version 450

#include "yave.glsl"


// https://64.github.io/tonemapping/
// https://mynameismjp.wordpress.com/2010/04/30/a-closer-look-at-tone-mapping/
// https://www.academia.edu/24772316/Programming_Vertex_Geometry_and_Pixel_Shaders_Screenshots_of_Alan_Wake_courtesy_of_Remedy_Entertainment
// https://imdoingitwrong.wordpress.com/2010/08/19/why-reinhard-desaturates-my-blacks-3/
// https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ToneMapping.hlsl

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(set = 0, binding = 1) uniform ToneMapping {
	ToneMappingParams params;
};

layout(set = 0, binding = 2) uniform KeyValue {
	float key_value;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;



vec3 gamma_correction(vec3 color) {
	const float gamma = 2.0;
	const float inv_gamma = 1.0 / gamma;

	return pow(color, vec3(inv_gamma));
}

// Gamma included
vec3 filmic_ALU(vec3 hdr) {
	const vec3 color = max(hdr - 0.004, vec3(0.0));
	return (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
}

float compute_exposure(float avg_lum) {
	return key_value / max(epsilon, avg_lum);
}


vec3 tone_map(vec3 hdr, float exposure) {
	const uint mode = 2;

	if(mode == 0) {
		return filmic_ALU(hdr * exposure);
	}

	if(mode == 1) {
		vec3 Yxy = RGB_to_Yxy(hdr);
		Yxy.x /= params.max_lum;
		const vec3 aces = ACES_fast(Yxy_to_RGB(Yxy));
		return gamma_correction(aces);
	}

	if(mode == 2) {
		const vec3 aces = ACES_fast(hdr * exposure);
		return gamma_correction(aces);
	}

	if(mode == 3) {
		const vec3 aces = ACES_fast(hdr);
		return gamma_correction(aces);
	}

	return hdr;
}

void main() {
	const ivec2 coord = ivec2(gl_FragCoord.xy);
	const vec3 hdr = texelFetch(in_color, coord, 0).rgb;
	const float exposure = compute_exposure(params.avg_lum);

	out_color = vec4(tone_map(hdr, exposure), 1.0);
}



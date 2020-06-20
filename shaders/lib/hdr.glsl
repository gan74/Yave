#ifndef HDR_GLSL
#define HDR_GLSL

#include "utils.glsl"

float reinhard(float lum, float white) {
	return lum * ((1.0 + lum) / sqr(white)) / (1.0 + lum);
}

vec3 reinhard(vec3 hdr, float white) {
	const float lum = luminance(hdr);
	const float scale = reinhard(lum, white) / lum;
	return hdr * scale;
}

float uncharted2(float hdr) {
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	return ((hdr * (A * hdr + C * B) + D * E) / (hdr * (A * hdr + B) + D * F)) - E / F;
}

vec3 uncharted2(vec3 hdr) {
	return vec3(uncharted2(hdr.r), uncharted2(hdr.g), uncharted2(hdr.b));
}

float ACES_fast(float hdr) {
	const float A = 2.51;
	const float B = 0.03;
	const float C = 2.43;
	const float D = 0.59;
	const float E = 0.14;
	return (hdr * (A * hdr + B)) / (hdr * (C * hdr + E) + E);
}

vec3 ACES_fast(vec3 hdr) {
	return vec3(ACES_fast(hdr.r), ACES_fast(hdr.g), ACES_fast(hdr.b));
}

#endif // HDR_GLSL
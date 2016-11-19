#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;

const float pi = 3.1415926535897932384626433832795;

float rand(vec2 co) {
	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec2 grd(vec2 c) {
	float gx = rand(c);
	gx = gx * 2.0 * pi;
	return /*normalize*/(vec2(cos(gx), sin(gx)));
}

float dotGrd(vec2 p, ivec2 c) {
	return dot(grd(vec2(c)), (p - vec2(c)));
}

float smoothStep(float x) {
	return x * x * (3.0 - 2.0 * x);
}

vec2 fade(vec2 x) {
	return vec2((smoothStep(x.x)), (smoothStep(x.y)));
}

float perlin(vec2 p) {
	ivec2 cell = ivec2(floor(p));
	ivec2 x0y0 = cell;
	ivec2 x1y0 = cell + ivec2(1, 0);
	ivec2 x0y1 = cell + ivec2(0, 1);
	ivec2 x1y1 = cell + ivec2(1, 1);
	float s = dotGrd(p, x0y0);
	float t = dotGrd(p, x1y0);
	float u = dotGrd(p, x0y1);
	float v = dotGrd(p, x1y1);
	vec2 faded = fade(fract(p));
	vec2 f2 = mix(vec2(s, u), vec2(t, v), faded.x);
	float f = mix(f2.x, f2.y, faded.y);
	return f ;
}

float ridged(vec2 p) {
	float c = abs(perlin(p));
	return pow(1.0 - c, 4.0);
}

float fbm(vec2 uv, int octaves) {
	const float uv_mul = 1.25;
	const float w_mul = 0.25;
	const mat2 rot = mat2(1.6, 1.2, -1.2, 1.6);
	
	float sum = 0.0;
	float w = 1.0;
	float total_w = 0.0;
	for(int i = 0; i < octaves; ++i) {
		sum += ridged(uv) * w;
		uv = rot * uv * uv_mul;
		total_w += w;
		w *= w_mul;
	}
	return sum / total_w;
}

float sqr(float x) {
	return x * x;
}

vec3 normal_from_height(vec2 uv, float h) {
	vec3 pos = vec3(uv, h);
	
	vec3 d_x = /*normalize*/(dFdx(pos));
	vec3 d_y = /*normalize*/(dFdy(pos));
	
	return normalize(cross(d_x, d_y));
}

void main() {
	float h = fbm(v_uv, 4);
	vec3 normal = normal_from_height(v_uv, h);
	
	vec3 light = normalize(vec3(0.0, 1.0, 1.0));
	float lambert = dot(light, normal);
	
	out_color = vec4(normal * 0.5 + 0.5, 0.0);
}

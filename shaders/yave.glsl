#ifndef YAVE_GLSL
#define YAVE_GLSL

#include "constants.glsl"
#include "sh.glsl"

// -------------------------------- TYPES --------------------------------

struct DirectionalLight {
	vec3 direction;
	uint padding_0;

	vec3 color;
	uint padding_1;
};

struct PointLight {
	vec3 position;
	float radius;

	vec3 color;
	float falloff;
};

struct SpotLight {
	vec3 position;
	float radius;

	vec3 color;
	float falloff;

	vec3 forward;
	float cos_angle;

	float angle_exp;
	uint shadow_map_index;

	uvec2 padding_0;
};

struct ShadowMapParams {
	mat4 view_proj;
	vec2 uv_offset;
	vec2 uv_mul;
};

struct LightingCamera {
	mat4 inv_matrix;
	vec3 position;
	uint padding_0;
	vec3 forward;
	uint padding_1;
};

struct Frustum {
	vec4 planes[6];
};

struct Frustum4 {
	vec4 planes[4];
};

struct ToneMappingParams {
	float avg_lum;
	float max_lum;

	ivec2 padding_0;
};


// -------------------------------- UTILS --------------------------------

bool is_OOB(float z) {
	return z <= 0.0; // reversed Z
}

float saturate(float x) {
	return min(1.0, max(0.0, x));
}

vec2 saturate(vec2 x) {
	return min(vec2(1.0), max(vec2(0.0), x));
}

vec3 saturate(vec3 x) {
	return min(vec3(1.0), max(vec3(0.0), x));
}

vec4 saturate(vec4 x) {
	return min(vec4(1.0), max(vec4(0.0), x));
}

float sqr(float x) {
	return x * x;
}

float noise(vec2 co) {
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

uint hash(uint x) {
	x += (x << 10u);
	x ^= (x >>  6u);
	x += (x <<  3u);
	x ^= (x >> 11u);
	x += (x << 15u);
	return x;
}

uint hash(uvec2 x) {
	return hash(x.x ^ hash(x.y));
}

float log10(float x) {
	return (1.0 / log(10.0)) * log(x);
}

mat4 indentity() {
	return mat4(1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1);
}

vec2 hammersley(uint i, uint N) {
	uint bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float radical_inverse = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) / float(N), radical_inverse);
}

vec2 to_equirec(vec3 v) {
	return -vec2(atan(-v.y, v.x), asin(v.z)) * vec2(0.1591, 0.3183) + vec2(0.5);
}

vec3 cube_dir(vec2 texCoord, uint side) {
	const vec2 tex = texCoord * 2.0 - 1.0;

	if(side == 0) return vec3(1.0, -tex.y, -tex.x); // front
	if(side == 1) return vec3(-1.0, -tex.y, tex.x); // back

	if(side == 2) return vec3(tex.x, 1.0, tex.y); // right
	if(side == 3) return vec3(tex.x, -1.0, -tex.y); // left

	if(side == 4) return vec3(tex.x, -tex.y, 1.0); // top
	if(side == 5) return vec3(-tex.x, -tex.y, -1.0); // bottom
}

float luminance(vec3 rgb) {
	return dot(rgb, vec3(0.2126, 0.7152, 0.0722));
}


// -------------------------------- SPECTRUM --------------------------------

vec3 spectrum(float x) {
	x = x * 6.0;
	if(x > 5.0) return vec3(1.0, 0.0, 6.0 - x);
	if(x > 4.0) return vec3(x - 4.0, 0.0, 1.0);
	if(x > 3.0) return vec3(0.0, 4.0 - x, 1.0);
	if(x > 2.0) return vec3(0.0, 1.0, x - 2.0);
	if(x > 1.0) return vec3(2.0 - x, 1.0, 0.0);
	return vec3(1.0, x, 0.0);
}

vec3 load_spectrum(float x) {
	x = (1.0 - x) * 4.0;
	if(x > 3.0) return vec3(0.0, 4.0 - x, 1.0);
	if(x > 2.0) return vec3(0.0, 1.0, x - 2.0);
	if(x > 1.0) return vec3(2.0 - x, 1.0, 0.0);
	return vec3(1.0, x, 0.0);
}


vec3 spectrum(uint x) {
	x = (x % 6) + 1;
	return vec3((x & 0x01) != 0 ? 1.0 : 0.0,
				(x & 0x02) != 0 ? 1.0 : 0.0,
				(x & 0x04) != 0 ? 1.0 : 0.0);
}


// -------------------------------- PROJECTION --------------------------------

vec3 unproject_ndc(vec3 ndc, mat4 inv_matrix) {
	const vec4 p = inv_matrix * vec4(ndc, 1.0);
	return p.xyz / p.w;
}

vec3 unproject(vec2 uv, float depth, mat4 inv_matrix) {
	const vec3 ndc = vec3(uv * 2.0 - vec2(1.0), depth);
	return unproject_ndc(ndc, inv_matrix);
}

vec3 project(vec3 pos, mat4 proj_matrix) {
	const vec4 p = proj_matrix * vec4(pos, 1.0);
	const vec3 p3 = p.xyz / p.w;
	return vec3(p3.xy * 0.5 + vec2(0.5), p3.z);
}


// -------------------------------- COLOR --------------------------------

// https://github.com/BruOp/bae/blob/master/examples/common/shaderlib.sh
vec3 RGB_to_XYZ(vec3 rgb) {
	// http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
	return vec3(
	    dot(vec3(0.4124564, 0.3575761, 0.1804375), rgb),
	    dot(vec3(0.2126729, 0.7151522, 0.0721750), rgb),
	    dot(vec3(0.0193339, 0.1191920, 0.9503041), rgb)
	);
}

vec3 XYZ_to_RGB(vec3 xyz) {
	return vec3(
	    dot(vec3( 3.2404542, -1.5371385, -0.4985314), xyz),
	    dot(vec3(-0.9692660,  1.8760108,  0.0415560), xyz),
	    dot(vec3( 0.0556434, -0.2040259,  1.0572252), xyz)
	);
}

vec3 XYZ_to_Yxy(vec3 xyz) {
	// http://www.brucelindbloom.com/index.html?Eqn_xyY_to_XYZ.html
	const float i = 1.0 / dot(xyz, vec3(1.0));
	return vec3(xyz.y, xyz.x * i, xyz.y * i);
}

vec3 Yxy_to_XYZ(vec3 Yxy) {
	return vec3(
	    Yxy.x * Yxy.y / Yxy.z,
	    Yxy.x,
	    Yxy.x * (1.0 - Yxy.y - Yxy.z) / Yxy.z
	);
}

vec3 RGB_to_Yxy(vec3 rgb) {
	return XYZ_to_Yxy(RGB_to_XYZ(rgb));
}

vec3 Yxy_to_RGB(vec3 Yxy) {
	return XYZ_to_RGB(Yxy_to_XYZ(Yxy));
}


// -------------------------------- HDR --------------------------------

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


// -------------------------------- LIGHTING --------------------------------

float attenuation(float distance, float radius) {
	const float x = min(distance, radius);
	return sqr(1.0 - sqr(sqr(x / radius))) / (sqr(x) + 1.0);
}

float attenuation(float distance, float radius, float falloff) {
	return attenuation(distance * falloff, radius * falloff);
}

// https://google.github.io/filament/Filament.html
float D_GGX(float NoH, float roughness) {
	const float a2 = sqr(sqr(roughness));
	const float denom = sqr(sqr(NoH) * (a2 - 1.0) + 1.0) * pi;
	return a2 / denom;
}

float G_sub_GGX(float NoV, float k) {
	const float denom = NoV * (1.0 - k) + k;
	return NoV / denom;
}

float G_GGX(float NoV, float NoL, float k) {
	// k for direct lighting = sqr(roughness + 1.0) / 8.0;
	// k for IBL             = sqr(roughness) / 2.0;

	const float ggx_v = G_sub_GGX(NoV, k);
	const float ggx_l = G_sub_GGX(NoL, k);
	return ggx_v * ggx_l;
}

vec3 approx_F0(float metallic, vec3 albedo) {
	return mix(vec3(0.04), albedo, metallic);
}

vec3 F_Schlick(float NoV, vec3 F0) {
	return F0 + (vec3(1.0) - F0) * pow(1.0 - NoV, 5.0);
}

vec3 F_Schlick(float NoV, vec3 F0, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - NoV, 5.0);
}

float brdf_cook_torrance(float NoH, float NoV, float NoL, float roughness) {
	const float D = D_GGX(NoH, roughness);

	const float k_direct = sqr(roughness + 1.0) / 8.0;
	const float G = G_GGX(NoV, NoL, k_direct);

	const float denom = 4.0 * NoL * NoV + epsilon;

	return D * G / denom;
}

float brdf_lambert() {
	return 1.0 / pi;
}

vec3 L0(vec3 normal, vec3 light_dir, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	// maybe investigate optimisations described here: http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
	const vec3 half_vec = normalize(light_dir + view_dir);

	const float NoH = max(0.0, dot(normal, half_vec));
	const float NoV = max(0.0, dot(normal, view_dir));
	const float NoL = max(0.0, dot(normal, light_dir));
	const float HoV = max(0.0, dot(half_vec, view_dir));

	const vec3 F = F_Schlick(HoV, approx_F0(metallic, albedo));

	const vec3 kS = F;
	const vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	const vec3 specular = kS * brdf_cook_torrance(NoH, NoV, NoL, roughness);
	// Won't this account for the albedo twice?
	const vec3 diffuse = kD * albedo * brdf_lambert();

	return (diffuse + specular) * NoL;
}

// -------------------------------- SHADOW --------------------------------

float variance_shadow(vec2 moments, float depth) {
	const float variance = sqr(moments.x) - moments.y;
	const float diff = moments.x - depth;
	const float p = variance / (variance + sqr(diff));
	return max(p, depth <= moments.x ? 1.0 : 0.0);
}

// -------------------------------- IBL --------------------------------

vec3 importance_sample_GGX(vec2 Xi, vec3 normal, float roughness) {
	const float a2 = sqr(sqr(roughness));

	const float phi = 2.0 * pi * Xi.x;
	const float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y));
	const float sin_theta = sqrt(1.0 - sqr(cos_theta));

	const vec3 half_vec = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

	const vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	const vec3 tangent = normalize(cross(up, normal));
	const vec3 bitangent = cross(normal, tangent);

	return normalize(tangent * half_vec.x + bitangent * half_vec.y + normal * half_vec.z);
}

vec2 integrate_brdf(float NoV, float roughness) {
	const vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0.0, NoV);

	vec2 integr = vec2(0.0);

	const vec3 N = vec3(0.0, 0.0, 1.0);
	const uint SAMPLE_COUNT = 1024;
	for(uint i = 0; i != SAMPLE_COUNT; ++i) {
		const vec2 Xi = hammersley(i, SAMPLE_COUNT);
		const vec3 H  = importance_sample_GGX(Xi, N, roughness);
		const vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		const float NoL = max(0.0, L.z);
		const float NoH = max(0.0, H.z);
		const float VoH = max(0.0, dot(V, H));

		if(NoL > 0.0) {
			const float k_ibl = sqr(roughness) * 0.5;
			const float G = G_GGX(NoV, NoL, k_ibl);
			const float G_Vis = (G * VoH) / (NoH * NoV);
			const float Fc = pow(1.0 - VoH, 5.0);

			integr += vec2((1.0 - Fc) * G_Vis, Fc * G_Vis);
		}
	}
	return integr / SAMPLE_COUNT;
}

vec3 ibl_irradiance(samplerCube probe, sampler2D brdf_lut, vec3 normal, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	const uint probe_mips = textureQueryLevels(probe);
	const float NoV = max(0.0, dot(normal, view_dir));

	const vec3 kS = F_Schlick(NoV, approx_F0(metallic, albedo), roughness);
	const vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	const vec3 irradiance = textureLod(probe, normal, probe_mips - 1).rgb;
	const vec3 diffuse = kD * irradiance * albedo;

	const vec3 reflected = reflect(-view_dir, normal);
	const vec2 brdf = texture(brdf_lut, vec2(NoV, roughness)).xy; // TODO: make it so we don't wrap (which breaks roughness close to 0 or 1)
	const vec3 specular = (kS * brdf.x + brdf.y) * textureLod(probe, reflected, roughness * probe_mips).rgb;

	return diffuse + specular;
}

vec3 sh_irradiance(SH probe, sampler2D brdf_lut, vec3 normal, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	const float NoV = max(0.0, dot(normal, view_dir));

	const vec3 kS = F_Schlick(NoV, approx_F0(metallic, albedo), roughness);
	const vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	const vec3 irradiance = eval_sh(probe, normal);
	const vec3 diffuse = kD * irradiance * albedo;

	const vec3 reflected = reflect(-view_dir, normal);
	const vec2 brdf = texture(brdf_lut, vec2(NoV, roughness)).xy; // TODO: make it so we don't wrap (which breaks roughness close to 0 or 1)
	const vec3 specular = (kS * brdf.x + brdf.y) * eval_sh(probe, reflected);

	return diffuse + specular;
}


// -------------------------------- G-BUFFER --------------------------------

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

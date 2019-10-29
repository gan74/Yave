#ifndef YAVE_GLSL
#define YAVE_GLSL

// -------------------------------- CONSTANTS --------------------------------

const float pi = 3.1415926535897932384626433832795;
const float euler = 2.7182818284590452353602874713526;
const float epsilon = 0.001;

const uint max_uint = uint(0xFFFFFFFF);

const uint max_bones = 256;
const uint max_tile_lights = 256;

const float lum_histogram_offset = 8.0;
const float lum_histogram_mul = 8.0;



// -------------------------------- TYPES --------------------------------

struct DirectionalLight {
	vec3 direction;
	float padding_0;

	vec3 color;
	float padding_1;
};

struct PointLight {
	vec3 position;
	float radius;

	vec3 color;
	float falloff;
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
	float avg_luminance;
	float max_lum;

	vec2 padding_0;
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
	float radical_inverse = float(bits) * 2.3283064365386963e-10;
	return vec2(float(i) / float(N), radical_inverse);
}

vec2 to_equirec(vec3 v) {
	return vec2(atan(-v.y, v.x), asin(v.z)) * vec2(0.1591, 0.3183) + vec2(0.5);
}

vec3 cube_dir(vec2 texCoord, uint side) {
	vec2 tex = texCoord * 2.0 - 1.0;

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
	vec4 p = inv_matrix * vec4(ndc, 1.0);
	return p.xyz / p.w;
}

vec3 unproject(vec2 uv, float depth, mat4 inv_matrix) {
	vec3 ndc = vec3(uv * 2.0 - vec2(1.0), depth);
	return unproject_ndc(ndc, inv_matrix);
}

vec3 project(vec3 pos, mat4 proj_matrix) {
	vec4 p = proj_matrix * vec4(pos, 1.0);
	vec3 p3 = p.xyz / p.w;
	return vec3(p3.xy * 0.5 + vec2(0.5), p3.z);
}


// -------------------------------- CULLING --------------------------------

bool is_inside(vec4 plane, vec3 pos, float radius) {
	return dot(vec4(pos, 1.0), plane) + radius > 0.0;
}

bool is_inside(Frustum frustum, vec3 pos, float radius) {
	for(uint i = 0; i != 6; ++i) {
		if(!is_inside(frustum.planes[i], pos, radius)) {
			return false;
		}
	}
	return true;
}

bool is_inside(Frustum4 frustum, vec3 pos, float radius) {
	for(uint i = 0; i != 4; ++i) {
		if(!is_inside(frustum.planes[i], pos, radius)) {
			return false;
		}
	}
	return true;
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
	float i = 1.0 / dot(xyz, vec3(1.0));
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
	float lum = luminance(hdr);
	float scale = reinhard(lum, white) / lum;
	return hdr * scale;
}

float uncharted2(float hdr) {
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((hdr * (A * hdr + C * B) + D * E) / (hdr * (A * hdr + B) + D * F)) - E / F;
}

vec3 uncharted2(vec3 hdr) {
	return vec3(uncharted2(hdr.r), uncharted2(hdr.g), uncharted2(hdr.b));
}

float ACES_fast(float hdr) {
	float A = 2.51;
	float B = 0.03;
	float C = 2.43;
	float D = 0.59;
	float E = 0.14;
	return (hdr * (A * hdr + B)) / (hdr * (C * hdr + E) + E);
}

vec3 ACES_fast(vec3 hdr) {
	return vec3(ACES_fast(hdr.r), ACES_fast(hdr.g), ACES_fast(hdr.b));
}

// -------------------------------- LIGHTING --------------------------------

float attenuation(float distance, float radius) {
	float x = min(distance, radius);
	return sqr(1.0 - sqr(sqr(x / radius))) / (sqr(x) + 1.0);
}

float attenuation(float distance, float radius, float falloff) {
	return attenuation(distance * falloff, radius * falloff);
}

/*vec3 reflection(samplerCube envmap, vec3 normal, vec3 view) {
	vec3 r = reflect(view, normal);
	return textureLod(envmap, r, 0).rgb;
}*/

float D_GGX(float NoH, float roughness) {
	float a2 = sqr(sqr(roughness));
	float denom = sqr(sqr(NoH) * (a2 - 1.0) + 1.0) * pi;
	return a2 / denom;
}

float G_sub_GGX(float NoV, float k) {
	float denom = NoV * (1.0 - k) + k;
	return NoV / denom;
}

float G_GGX(float NoV, float NoL, float k) {
	// k for direct lighting = sqr(roughness + 1.0) / 8.0;
	// k for IBL             = sqr(roughness) / 2.0;

	float ggx_v = G_sub_GGX(NoV, k);
	float ggx_l = G_sub_GGX(NoL, k);
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
	float D = D_GGX(NoH, roughness);

	float k_direct = sqr(roughness + 1.0) / 8.0;
	float G = G_GGX(NoV, NoL, k_direct);

	float denom = 4.0 * NoL * NoV + epsilon;

	return D * G / denom;
}

float brdf_lambert() {
	return 1.0 / pi;
}

vec3 L0(vec3 normal, vec3 light_dir, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	vec3 half_vec = normalize(light_dir + view_dir);
	float NoH = max(0.0, dot(normal, half_vec));
	float NoV = max(0.0, dot(normal, view_dir));
	float NoL = max(0.0, dot(normal, light_dir));
	float HoV = max(0.0, dot(half_vec, view_dir));

	vec3 F = F_Schlick(HoV, approx_F0(metallic, albedo));

	vec3 kS = F;
	vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	vec3 specular = kS * brdf_cook_torrance(NoH, NoV, NoL, roughness);
	vec3 diffuse = kD * albedo * brdf_lambert();

	return (diffuse + specular) * NoL;
}

// -------------------------------- SHADOW --------------------------------

float variance_shadow(vec2 moments, float depth) {
	float variance = sqr(moments.x) - moments.y;
	float diff = moments.x - depth;
	float p = variance / (variance + sqr(diff));
	return max(p, depth <= moments.x ? 1.0 : 0.0);
}

// -------------------------------- IBL --------------------------------

vec3 importance_sample_GGX(vec2 Xi, vec3 normal, float roughness) {
	float a2 = sqr(sqr(roughness));

	float phi = 2.0 * pi * Xi.x;
	float cos_theta = sqrt((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y));
	float sin_theta = sqrt(1.0 - sqr(cos_theta));

	vec3 half_vec = vec3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 bitangent = cross(normal, tangent);

	return normalize(tangent * half_vec.x + bitangent * half_vec.y + normal * half_vec.z);
}

vec2 integrate_brdf(float NoV, float roughness) {
	vec3 V = vec3(sqrt(1.0 - NoV * NoV), 0.0, NoV);

	vec2 integr = vec2(0.0);

	vec3 N = vec3(0.0, 0.0, 1.0);
	const uint SAMPLE_COUNT = 1024;
	for(uint i = 0; i != SAMPLE_COUNT; ++i) {
		vec2 Xi = hammersley(i, SAMPLE_COUNT);
		vec3 H  = importance_sample_GGX(Xi, N, roughness);
		vec3 L  = normalize(2.0 * dot(V, H) * H - V);

		float NoL = max(0.0, L.z);
		float NoH = max(0.0, H.z);
		float VoH = max(0.0, dot(V, H));

		if(NoL > 0.0) {
			float k_ibl = sqr(roughness) * 0.5;
			float G = G_GGX(NoV, NoL, k_ibl);
			float G_Vis = (G * VoH) / (NoH * NoV);
			float Fc = pow(1.0 - VoH, 5.0);

			integr += vec2((1.0 - Fc) * G_Vis, Fc * G_Vis);
		}
	}
	return integr / SAMPLE_COUNT;
}

vec3 ibl_irradiance(samplerCube probe, sampler2D brdf_lut, vec3 normal, vec3 view_dir, float roughness, float metallic, vec3 albedo) {
	uint probe_mips = textureQueryLevels(probe);
	float NoV = max(0.0, dot(normal, view_dir));

	vec3 kS = F_Schlick(NoV, approx_F0(metallic, albedo), roughness);
	vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

	vec3 irradiance = textureLod(probe, normal, probe_mips - 1).rgb;
	vec3 diffuse = kD * irradiance * albedo;

	vec3 reflected = reflect(-view_dir, normal);
	vec2 brdf = texture(brdf_lut, vec2(NoV, roughness)).xy; // TODO: make it so we don't wrap (which breaks roughness close to 0 or 1)
	vec3 specular = (kS * brdf.x + brdf.y) * textureLod(probe, reflected, roughness * probe_mips).rgb;

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

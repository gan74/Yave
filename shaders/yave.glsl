
// -------------------------------- CONSTANTS --------------------------------

const float pi = 3.1415926535897932384626433832795;
const float epsilon = 0.001;

const uint max_bones = 256;
const uint max_tile_lights = 128;


// -------------------------------- TYPES --------------------------------

struct Light {
	vec3 position;
	float radius;
	vec3 color;
	uint type;
};

struct Frustum {
	vec4 planes[6];
};


// -------------------------------- UTILS --------------------------------

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
	return fract(sin(dot(co.xy, vec2(12.9898,78.233))) * 43758.5453);
}

vec3 spectrum(float x) {
	float wl = x * 400.0 + 380.0 ;
	if(wl > 645.0) return vec3(1.0, 0.0, 0.0);
	if(wl > 580.0) return vec3(1.0, (645.0 - wl) / 65.0, 0.0);
	if(wl > 510.0) return vec3((wl - 510.0) / 70.0, 1.0, 0.0);
	if(wl > 490.0) return vec3(0.0, 1.0, (510.0 - wl) / 20.0);
	if(wl > 440.0) return vec3(0.0, (wl - 440.0) / 40.0, 1.0);
	if(wl > 380.0) return vec3((440 - wl) / 60.0, 0.0, 1.0);
	return vec3(1.0);
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


// -------------------------------- CULLING --------------------------------

bool is_inside(Frustum frustum, vec3 pos, float radius) {
	for(uint i = 0; i != 6; ++i) {
		if(dot(vec4(pos, 1.0), frustum.planes[i]) + radius < 0.0) {
			return false;
		}
	}
	return true;
}

bool is_inside(vec4 plane, vec3 pos, float radius) {
	return dot(vec4(pos, 1.0), plane) + radius < 0.0;
}

bool is_inside_4(Frustum frustum, vec3 pos, float radius) {
	for(uint i = 0; i != 4; ++i) {
		if(dot(vec4(pos, 1.0), frustum.planes[i]) + radius < 0.0) {
			return false;
		}
	}
	return true;
}


// -------------------------------- HDR --------------------------------

vec3 reinhard(vec3 hdr, float k) {
	return hdr / (hdr + k);
}

vec3 reinhard(vec3 hdr) {
	return reinhard(hdr, 1.0);
}


// -------------------------------- LIGHTING --------------------------------

float attenuation(float distance, float radius) {
	float x = min(distance, radius);
	return sqr(1.0 - sqr(sqr(x / radius))) / (sqr(x) + 1.0);
}

float brdf(vec3 normal, vec3 light_dir, vec3 view_dir) {
	float lambert = saturate(dot(normal, light_dir));

	vec3 half_vec = normalize(light_dir + view_dir);
	float phong = pow(saturate(dot(normal, half_vec)), 32.0);

	return lambert + phong;
}



// -------------------------------- CONSTANTS --------------------------------

const float pi = 3.1415926535897932384626433832795;
const float epsilon = 0.001;


const uint DirectionalLight = 0;
const uint PointLight = 1;


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


// -------------------------------- PROJECTION --------------------------------

vec3 unproject(vec2 uv, float depth, mat4 inv_matrix) {
	vec3 ndc = vec3(uv * 2.0 - vec2(1.0), depth);
	vec4 p = inv_matrix * vec4(ndc, 1.0);
	return p.xyz / p.w;
}


// -------------------------------- CULLING --------------------------------

bool is_inside(Frustum frustum, vec3 pos, float radius) {
	for(uint i = 0; i < 6; ++i) {
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

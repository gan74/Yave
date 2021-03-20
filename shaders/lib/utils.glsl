#ifndef UTILS_GLSL
#define UTILS_GLSL

#include "constants.glsl"

// -------------------------------- TYPES --------------------------------

struct Frustum {
    vec4 planes[6];
};

struct Frustum4 {
    vec4 planes[4];
};

struct Camera {
    mat4 view_proj;
    mat4 inv_view_proj;

    vec3 position;
    uint padding_0;

    vec3 forward;
    uint padding_1;

    vec3 up;
    uint padding_2;

    float z_near;
    float z_far;
    uvec2 padding_3;
};

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

    vec3 encl_sphere_center;
    float encl_sphere_radius;

    float angle_exp;
    uint shadow_map_index;

    uvec2 padding_0;
};

struct ShadowMapParams {
    mat4 view_proj;
    vec2 uv_offset;
    vec2 uv_mul;

    float size;
    float texel_size;
    uvec2 padding_0;
};

struct ToneMappingParams {
    float exposure;
    float avg_lum;
    float max_lum;

    uint padding_0;
};

struct Surfel {
    vec3 position;
    uint padding_0;

    vec3 albedo;
    uint padding_1;

    vec3 normal;
    uint padding_2;
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

bool intersect_ray_sphere(vec3 origin, vec3 dir, float radius, out vec2 intersections) {
    const float a = dot(dir, dir);
    const float b = 2.0 * dot(dir, origin);
    const float c = dot(origin, origin) - sqr(radius);
    const float d = sqr(b) - 4.0 * a * c;

    if (d < 0.0) {
        intersections = vec2(1e5, -1e5);
        return false;
    }

    intersections = vec2(
            (-b - sqrt(d)) / (2.0 * a),
            (-b + sqrt(d)) / (2.0 * a)
        );

    return true;
}

vec4 plane(vec3 p0, vec3 p1, vec3 p2) {
    const vec3 n = normalize(cross(p0 - p1, p2 - p1));
    return vec4(-n, dot(n, p1));
}

bool is_inside(Frustum4 frustum, vec3 pos, float radius) {
    // This is far from optimal, but it will do for now
    for(uint i = 0; i != 4; ++i) {
        if(dot(vec4(pos, 1.0), frustum.planes[i]) > radius) {
            return false;
        }
    }
    return true;
}

// Returns (near, dist exit)
vec2 intersect_sphere(vec3 center, float radius, vec3 origin, vec3 dir) {
    const vec3 offset = origin - center;
    const float a = 1.0;
    const float b = dot(offset, dir) * 2.0;
    const float c = dot(offset, offset) - (radius * radius);
    float d = b * b - 4 * a * c;

    if(d > 0.0) {
        const float s = sqrt(d);
        const float two_a = 2.0 * a;
        const float near = max(0.0, -(b + s) / two_a);
        const float far = (-b + s) / two_a;

        if(far >= 0.0) {
            return vec2(near, far - near);
        }
    }
    return vec2(-1.0, 0.0);
}

vec3 unpack_normal_map(vec2 normal) {
    normal = normal * 2.0 - vec2(1.0);
    return vec3(normal, 1.0 - sqrt(dot(normal, normal)));
}

vec3 create_perpendicular(vec3 v) {
    vec3 t = vec3(1, 0, 0);
    if(abs(v.x) > 0.99) {
        t = vec3(0, 1, 0);
    }
    return cross(t, v);
}

float intersection_distance(vec4 plane, vec3 origin, vec3 dir) {
    return -(dot(origin, plane.xyz) + plane.w) / dot(dir, plane.xyz);
}

vec3 intersection_point(vec4 plane, vec3 origin, vec3 dir) {
    return origin + intersection_distance(plane, origin, dir) * dir;
}

vec4 unpack_color(uint packed) {
    return vec4(
        (packed >> 0) & 0xFF,
        (packed >> 8) & 0xFF,
        (packed >> 16) & 0xFF,
        (packed >> 24) & 0xFF
    ) / 255.0;
}

// -------------------------------- sRGB --------------------------------

float sRGB_to_linear(float x) {
    if(x <= 0.04045) {
        return x / 12.92;
    }
    return pow((x + 0.055) / 1.055, 2.4);
}

float linear_to_sRGB(float x) {
    if(x <= 0.0031308) {
        return x * 12.92;
    }
    return 1.055 * pow(x, 1.0 / 2.4) - 0.055;
}

vec3 sRGB_to_linear(vec3 v) {
    return vec3(sRGB_to_linear(v.r), sRGB_to_linear(v.g), sRGB_to_linear(v.b));
}

vec3 linear_to_sRGB(vec3 v) {
    return vec3(linear_to_sRGB(v.r), linear_to_sRGB(v.g), linear_to_sRGB(v.b));
}

vec4 sRGB_to_linear(vec4 v) {
    return vec4(sRGB_to_linear(v.rgb), v.a);
}

vec4 linear_to_sRGB(vec4 v) {
    return vec4(linear_to_sRGB(v.rgb), v.a);
}


// -------------------------------- BLOOM --------------------------------

vec3 bloom_threshold(vec3 hdr, float power, float threshold, float rev_threshold) {
    const float hdr_lum = luminance(hdr);

    const float hdr_weight = 1.0 / (1.0 + hdr_lum);
    const float lum = hdr_lum * hdr_weight;

    const float thresholded = max(0.0, (lum - threshold) * rev_threshold);
    const float bloom_factor = pow(thresholded, power);

    return hdr * bloom_factor * hdr_weight;
}

vec3 bloom_threshold(vec3 hdr, float power, float threshold) {
    return bloom_threshold(hdr, power, threshold, 1.0 / (1.0 - threshold));
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

float linearize(float depth, float near, float far) {
    // https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/MiniEngine/Core/Shaders/LinearizeDepthCS.hlsl
    const float z_magic = (far - near) / near;
    return saturate(1.0 / (z_magic * depth + 1.0));
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


#endif


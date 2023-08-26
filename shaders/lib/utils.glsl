#ifndef UTILS_GLSL
#define UTILS_GLSL

#include "constants.glsl"

// -------------------------------- TYPES --------------------------------

struct SurfaceInfo {
    vec3 albedo;

    float perceptual_roughness;
    float roughness;
    float metallic;

    vec3 F0;

    vec3 normal;
};
struct Frustum {
    vec4 planes[6];
};

struct Frustum4 {
    vec4 planes[4];
};

struct Camera {
    mat4 view_proj;
    mat4 inv_view_proj;

    mat4 proj;
    mat4 inv_proj;

    mat4 view;
    mat4 inv_view;

    vec3 position;
    uint padding_0;

    vec3 forward;
    uint padding_1;

    vec3 up;
    uint padding_2;
};

struct DirectionalLight {
    vec3 direction;
    float cos_disk;

    vec3 color;
    uint padding_1;

    uvec4 shadow_map_indices;
};

struct PointLight {
    vec3 position;
    float range;

    vec3 color;
    float falloff;

    vec3 padding_0;
    float min_radius;
};

struct SpotLight {
    vec3 position;
    float range;

    vec3 color;
    float falloff;

    vec3 forward;
    float min_radius;

    vec2 att_scale_offset;
    float sin_angle;
    uint shadow_map_index;

    vec3 encl_sphere_center;
    float encl_sphere_radius;
};

struct ShadowMapParams {
    mat4 view_proj;

    vec2 uv_offset;
    vec2 uv_mul;

    float size;
    float texel_size;
    uint padding_0;
    uint padding_1;
};

struct ExposureParams {
    float exposure;
    float avg_lum;
    float max_lum;

    uint padding_0;
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
            return vec2(near, far);
        }
    }
    return vec2(-1.0, 0.0);
}

// https://iquilezles.org/articles/intersectors/
bool intersect_ray_sphere(vec3 ray_origin, vec3 dir, vec3 pos, float radius, out vec2 intersections) {
    const vec3 oc = ray_origin - pos;
    const float b = -dot(oc, dir);
    const float c = dot(oc, oc) - sqr(radius);
    float h = sqr(b) - c;
    if(h < 0.0) {
        intersections = vec2(0.0);
        return false;
    }

    h = sqrt(h);
    intersections = vec2(b - h, b + h);
    return true;
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

vec4 unpack_2_10_10_10(uint packed) {
    return vec4(
        (vec3(
            (packed >> 20) & 0x03FF,
            (packed >> 10) & 0x03FF,
            (packed >>  0) & 0x03FF
        ) / float(0x03FF)) * 2.0 - 1.0,
        (packed >> 30 == 0) ? 1.0 : -1.0
    );
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

vec3 view_direction(Camera camera, vec2 uv) {
    return normalize(camera.position - unproject(uv, 0.5, camera.inv_view_proj));
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

vec3 HSV_to_RGB(float h, float s, float v) {
    h = fract(h) * 6.0;
    const int i = int(h);
    const float f = h - float(i);
    const float p = v * (1.0 - s);
    const float q = v * (1.0 - s * f);
    const float t = v * (1.0 - s * (1.0 - f));

    switch(i) {
        case 0: return vec3(v, t, p);
        case 1: return vec3(q, v, p);
        case 2: return vec3(p, v, t);
        case 3: return vec3(p, q, v);
        case 4: return vec3(t, p, v);

        default:
        break;
    }
    return vec3(v, p, q);
}

vec3 heat_spectrum(float x) {
    x *= 4.0;
    vec3 color = mix(vec3(0, 0, 1), vec3(0, 1, 0), saturate(x));
    color = mix(color, vec3(1, 1, 0), saturate(x - 1.0));
    color = mix(color, vec3(1, 0, 0), saturate(x - 2.0));
    color = mix(color, vec3(1, 1, 1), saturate(x - 3.0));
    return color / max(color.x, max(color.y, color.z));
}

vec3 uv_debug_color(vec2 p) {
    vec3 p3 = fract(p.xyx * vec3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return fract((p3.xxy + p3.yzz) * p3.zyx);
}

// -------------------------------- GBUFFER --------------------------------

vec3 approx_F0(float metallic, vec3 albedo) {
    return mix(vec3(0.04), albedo, metallic);
}

// https://knarkowicz.wordpress.com/2014/04/16/octahedron-normal-vector-encoding/
vec2 octahedron_wrap(vec2 v) {
    return (vec2(1.0) - abs(v.yx)) * vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
}

vec2 octahedron_encode(vec3 n) {
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : octahedron_wrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

vec3 octahedron_decode(vec2 f) {
    f = f * 2.0 - 1.0;
    // https://twitter.com/Stubbesaurus/status/937994790553227264
    vec3 n = vec3(f.xy, 1.0 - abs(f.x) - abs(f.y));
    const float t = saturate(-n.z);
    n.xy += vec2(n.x >= 0.0 ? -t : t, n.y >= 0.0 ? -t : t);
    return normalize(n);
}

#endif


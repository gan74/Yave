#ifndef SURFELS_GLSL
#define SURFELS_GLSL

#extension GL_EXT_shader_image_int64 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

#include "utils.glsl"

struct Surfel {
    vec3 pos;
    vec3 norm;
    vec2 uv;
    vec3 padding;
    float area;
};

struct InstanceData {
    mat4 model;

    vec3 center;
    float radius;

    float scale;
    float surfel_area;
    uint surfel_count;
    uint surfel_offset;
};





uint64_t pack_64(uvec2 dat) {
    uint64_t val = dat.x;
    val = (val << 32) | dat.y;
    return val;
}

uvec2 unpack_64(uint64_t val) {
    return uvec2(uint(val >> 32), uint(val));
}

vec3 build_tangent(vec3 n) {
    if(abs(n.x) > 0.9) {
        return vec3(0.0, 1.0, 0.0);
    }
    return vec3(1.0, 0.0, 0.0);
}

vec3 project_surfel(Surfel surfel, vec3 position) {
    const vec3 to_probe = (position - surfel.pos);
    const float probe_dist = length(to_probe);
    const vec3 probe_dir = to_probe / probe_dist;

    /*if(dot(surfel.norm, to_probe) > 0.0) {
        continue;
    }*/

    const vec2 uv = octahedron_encode(probe_dir);
    return vec3(uv, probe_dist);
}

#endif

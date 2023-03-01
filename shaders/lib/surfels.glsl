
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct SurfelData {
    vec4 a;
    vec4 b;
};

struct Surfel {
    vec3 pos;
    vec3 norm;
};

struct InstanceData {
    mat4 model;

    vec3 center;
    float radius;

    float padding;
    float surfel_area;
    uint surfel_count;
    uint surfel_offset;
};


Surfel to_surfel(SurfelData data) {
    Surfel surf;
    surf.pos = data.a.xyz;
    surf.norm = vec3(data.a.w, data.b.xy);
    return surf;
}


uint64_t pack_64(uvec2 dat) {
    uint64_t val = dat.x;
    val = (val << 32) | dat.y;
    return val;
}

uvec2 unpack_64(uint64_t val) {
    return uvec2(uint(val >> 32), uint(val));
}




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

    vec2 padding_0;
    uint surfel_count;
    uint surfel_offset;
};


Surfel to_surfel(SurfelData data) {
    Surfel surf;
    surf.pos = data.a.xyz;
    surf.norm = vec3(data.a.w, data.b.xy);
    return surf;
}


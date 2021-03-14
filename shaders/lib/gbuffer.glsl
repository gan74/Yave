#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL


struct GBufferData {
    vec3 albedo;
    float metallic;
    vec3 normal;
    float roughness;
};

struct RawGBuffer {
    vec4 rt0;
    vec4 rt1;
};


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



RawGBuffer encode_gbuffer(GBufferData data) {
    RawGBuffer raw;
    raw.rt0 = pack_color(data.albedo, data.metallic);
    raw.rt1 = pack_normal(data.normal, data.roughness);
    return raw;
}

void write_gbuffer(RawGBuffer raw, out vec4 rt0, out vec4 rt1) {
    rt0 = raw.rt0;
    rt1 = raw.rt1;
}

void write_gbuffer(GBufferData data, out vec4 rt0, out vec4 rt1) {
    write_gbuffer(encode_gbuffer(data), rt0, rt1);
}


GBufferData decode_gbuffer(RawGBuffer raw) {
    GBufferData data;
    unpack_color(raw.rt0, data.albedo, data.metallic);
    unpack_normal(raw.rt1, data.normal, data.roughness);
    return data;
}

void read_gbuffer(out RawGBuffer raw, vec4 rt0, vec4 rt1) {
    raw.rt0 = rt0;
    raw.rt1 = rt1;
}

GBufferData read_gbuffer(vec4 rt0, vec4 rt1) {
    RawGBuffer raw;
    read_gbuffer(raw, rt0, rt1);
    return decode_gbuffer(raw);
}

#endif


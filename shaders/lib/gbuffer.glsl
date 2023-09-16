#ifndef GBUFFER_GLSL
#define GBUFFER_GLSL

#include "utils.glsl"


struct RawGBuffer {
    vec4 rt0;
    vec4 rt1;
};

vec4 pack_color(vec3 color, float metallic) {
    return vec4(color, metallic);
}

vec4 pack_normal(vec3 normal, float roughness) {
    // return vec4(normalize(normal) * 0.5 + vec3(0.5), roughness);
    return vec4(octahedron_encode(normal), roughness, 0.0);
}

void unpack_color(vec4 buff, out vec3 color, out float metallic) {
    color = buff.rgb;
    metallic = buff.a;
}

void unpack_normal(vec4 buff, out vec3 normal, out float roughness) {
    // normal = normalize(buff.xyz * 2.0 - vec3(1.0));
    normal = octahedron_decode(buff.xy);
    roughness = max(0.05, buff.z);
}





RawGBuffer encode_gbuffer(SurfaceInfo info) {
    RawGBuffer raw;

    raw.rt0 = pack_color(info.albedo, info.metallic);
    raw.rt1 = pack_normal(info.normal, info.perceptual_roughness);

    return raw;
}

SurfaceInfo decode_gbuffer(RawGBuffer raw) {
    SurfaceInfo surface;

    unpack_color(raw.rt0, surface.albedo, surface.metallic);
    unpack_normal(raw.rt1, surface.normal, surface.perceptual_roughness);

    // https://blender.stackexchange.com/a/93792
    // surface.roughness = sqr(surface.perceptual_roughness);

    surface.alpha = sqr(surface.perceptual_roughness);
    surface.sqr_alpha = sqr(surface.alpha);

    surface.F0 = mix(vec3(0.04), surface.albedo, surface.metallic);

    return surface;
}





void write_gbuffer(RawGBuffer raw, out vec4 rt0, out vec4 rt1) {
    rt0 = raw.rt0;
    rt1 = raw.rt1;
}

void write_gbuffer(SurfaceInfo surface, out vec4 rt0, out vec4 rt1) {
    write_gbuffer(encode_gbuffer(surface), rt0, rt1);
}

void read_gbuffer(out RawGBuffer raw, vec4 rt0, vec4 rt1) {
    raw.rt0 = rt0;
    raw.rt1 = rt1;
}

SurfaceInfo read_gbuffer(vec4 rt0, vec4 rt1) {
    RawGBuffer raw;
    read_gbuffer(raw, rt0, rt1);
    return decode_gbuffer(raw);
}


#endif


#ifndef SHADOW_GLSL
#define SHADOW_GLSL


vec2 atlas_uv(ShadowMapInfo info, vec2 uv) {
    return info.uv_offset + uv * info.uv_mul;
}

float sample_shadow(sampler2DShadow shadow_map, vec3 proj, ShadowMapInfo info, float bias) {
    const vec2 uv = atlas_uv(info, proj.xy);
    return texture(shadow_map, vec3(uv, proj.z + bias)).x;
}

float compute_shadow_hard(sampler2DShadow shadow_map, ShadowMapInfo info, vec3 world_pos, float bias) {
    const vec3 proj = project(world_pos, info.view_proj);

    if(saturate(proj) != proj) {
        return 1.0;
    }

    return sample_shadow(shadow_map, proj, info, bias);
}

float compute_shadow_pcf(sampler2DShadow shadow_map, ShadowMapInfo info, vec3 world_pos, float bias) {
    const vec3 proj = project(world_pos, info.view_proj);

    if(saturate(proj) != proj) {
        return 1.0;
    }

    const vec2 offset = vec2(0.5);
    const vec2 tx = proj.xy * info.size + offset;
    const vec2 base = (floor(tx) - offset) * info.texel_size;
    const vec2 st = fract(tx);

#if 0
    const vec2 uw = vec2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    const vec2 vw = vec2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    const vec2 u = vec2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0) * info.texel_size;
    const vec2 v = vec2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0) * info.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.x), proj.z), info, bias);
    sum += uw.y * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.x), proj.z), info, bias);
    sum += uw.x * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.y), proj.z), info, bias);
    sum += uw.y * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.y), proj.z), info, bias);
    return sum / 16.0;
#else
    const vec3 uw = vec3(4.0 - 3.0 * st.x, 7.0, 1.0 + 3.0 * st.x);
    const vec3 vw = vec3(4.0 - 3.0 * st.y, 7.0, 1.0 + 3.0 * st.y);

    const vec3 u = vec3((3.0 - 2.0 * st.x) / uw.x - 2.0, (3.0 + st.x) / uw.y, st.x / uw.z + 2.0) * info.texel_size;
    const vec3 v = vec3((3.0 - 2.0 * st.y) / vw.x - 2.0, (3.0 + st.y) / vw.y, st.y / vw.z + 2.0) * info.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.x), proj.z), info, bias);
    sum += uw.y * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.x), proj.z), info, bias);
    sum += uw.z * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.z, v.x), proj.z), info, bias);
    sum += uw.x * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.y), proj.z), info, bias);
    sum += uw.y * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.y), proj.z), info, bias);
    sum += uw.z * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.z, v.y), proj.z), info, bias);
    sum += uw.x * vw.z * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.z), proj.z), info, bias);
    sum += uw.y * vw.z * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.z), proj.z), info, bias);
    sum += uw.z * vw.z * sample_shadow(shadow_map, vec3(base + vec2(u.z, v.z), proj.z), info, bias);
    return sum / 144.0;
#endif
}


float compute_shadow_hard(sampler2DShadow shadow_map, ShadowMapInfo info, vec3 world_pos) {
    return compute_shadow_hard(shadow_map, info, world_pos, 0.0);
}

float compute_shadow_pcf(sampler2DShadow shadow_map, ShadowMapInfo info, vec3 world_pos) {
    return compute_shadow_pcf(shadow_map, info, world_pos, 0.0);
}


#endif


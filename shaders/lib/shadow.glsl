#ifndef SHADOW_GLSL
#define SHADOW_GLSL

float sample_shadow(sampler2DShadow shadow_map, vec2 uvs, float proj_z, float bias_scale) {
    // Using derivatives cause artefacting around big depth discontinuities
    // const float bias = fwidth(proj_z) * bias_scale;

    const float bias = 0.0;
    return texture(shadow_map, vec3(uvs, proj_z + bias)).x;
}

float compute_bias_scale(ShadowMapParams params, vec2 uvs) {
    const float duv = min(fwidth(uvs.x), fwidth(uvs.y));
    const float texel_size = params.uv_mul.x;
    return params.texel_size / duv;
}

vec2 atlas_uv(ShadowMapParams params, vec2 uv) {
    return params.uv_offset + uv * params.uv_mul;
}

float compute_shadow_hard(sampler2DShadow shadow_map, ShadowMapParams params, vec3 world_pos) {
    const vec3 proj = project(world_pos, params.view_proj);
    const vec2 uv = atlas_uv(params, proj.xy);

    const float bias_scale = compute_bias_scale(params, uv);
    return sample_shadow(shadow_map, uv, proj.z, bias_scale);
}

float compute_shadow_pcf(sampler2DShadow shadow_map, ShadowMapParams params, vec3 world_pos) {
    const vec3 proj = project(world_pos, params.view_proj);
    const float bias_scale = compute_bias_scale(params, atlas_uv(params, proj.xy));

    const vec2 offset = vec2(0.5);
    const vec2 tx = proj.xy * params.size + offset;
    const vec2 base = (floor(tx) - offset) * params.texel_size;
    const vec2 st = fract(tx);

#if 0
    const vec2 uw = vec2(3.0 - 2.0 * st.x, 1.0 + 2.0 * st.x);
    const vec2 vw = vec2(3.0 - 2.0 * st.y, 1.0 + 2.0 * st.y);

    const vec2 u = vec2((2.0 - st.x) / uw.x - 1.0, st.x / uw.y + 1.0) * params.texel_size;
    const vec2 v = vec2((2.0 - st.y) / vw.x - 1.0, st.y / vw.y + 1.0) * params.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.x, v.x)), proj.z, bias_scale);
    sum += uw.y * vw.x * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.y, v.x)), proj.z, bias_scale);
    sum += uw.x * vw.y * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.x, v.y)), proj.z, bias_scale);
    sum += uw.y * vw.y * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.y, v.y)), proj.z, bias_scale);
    return sum / 16.0;
#else
    const vec3 uw = vec3(4.0 - 3.0 * st.x, 7.0, 1.0 + 3.0 * st.x);
    const vec3 vw = vec3(4.0 - 3.0 * st.y, 7.0, 1.0 + 3.0 * st.y);

    const vec3 u = vec3((3.0 - 2.0 * st.x) / uw.x - 2.0, (3.0 + st.x) / uw.y, st.x / uw.z + 2.0) * params.texel_size;
    const vec3 v = vec3((3.0 - 2.0 * st.y) / vw.x - 2.0, (3.0 + st.y) / vw.y, st.y / vw.z + 2.0) * params.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.x, v.x)), proj.z, bias_scale);
    sum += uw.y * vw.x * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.y, v.x)), proj.z, bias_scale);
    sum += uw.z * vw.x * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.z, v.x)), proj.z, bias_scale);
    sum += uw.x * vw.y * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.x, v.y)), proj.z, bias_scale);
    sum += uw.y * vw.y * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.y, v.y)), proj.z, bias_scale);
    sum += uw.z * vw.y * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.z, v.y)), proj.z, bias_scale);
    sum += uw.x * vw.z * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.x, v.z)), proj.z, bias_scale);
    sum += uw.y * vw.z * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.y, v.z)), proj.z, bias_scale);
    sum += uw.z * vw.z * sample_shadow(shadow_map, atlas_uv(params, base + vec2(u.z, v.z)), proj.z, bias_scale);
    return sum / 144.0;
#endif
}


#endif


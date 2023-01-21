#ifndef SHADOW_GLSL
#define SHADOW_GLSL

vec3 compute_depth_normal(vec3 world_pos, SurfaceInfo surface) {
    const vec3 dx = dFdx(world_pos);
    const vec3 dy = dFdy(world_pos);
    const vec3 depth_normal = normalize(cross(dy, dx));
    return dot(depth_normal, surface.normal) > epsilon ? depth_normal : surface.normal;
}


float compute_automatic_bias(ShadowMapParams params, vec3 depth_normal, vec3 light_direction) {
    const float NoL = max(0.0, dot(depth_normal, light_direction));
    const float bias = max(params.base_bias * (1.0 - NoL), params.base_bias * 0.1);
    return bias * params.texel_size;
}



vec2 atlas_uv(ShadowMapParams params, vec2 uv) {
    return params.uv_offset + uv * params.uv_mul;
}

float sample_shadow(sampler2DShadow shadow_map, vec3 proj, ShadowMapParams params, float bias) {
    const vec2 uv = atlas_uv(params, proj.xy);
    // Using derivatives can cause artefacting around big depth discontinuities
    // const float bias = fwidth(proj.z) / max(fwidth(proj.x), fwidth(proj.y));
    return texture(shadow_map, vec3(uv, proj.z + bias)).x;
}

float compute_shadow_hard(sampler2DShadow shadow_map, ShadowMapParams params, vec3 world_pos, float bias) {
    const vec3 proj = project(world_pos, params.view_proj);

    if(saturate(proj) != proj) {
        return 1.0;
    }

    return sample_shadow(shadow_map, proj, params, bias);
}

float compute_shadow_pcf(sampler2DShadow shadow_map, ShadowMapParams params, vec3 world_pos, float bias) {
    const vec3 proj = project(world_pos, params.view_proj);

    if(saturate(proj) != proj) {
        return 1.0;
    }

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
    sum += uw.x * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.x), proj.z), params, bias);
    sum += uw.y * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.x), proj.z), params, bias);
    sum += uw.x * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.y), proj.z), params, bias);
    sum += uw.y * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.y), proj.z), params, bias);
    return sum / 16.0;
#else
    const vec3 uw = vec3(4.0 - 3.0 * st.x, 7.0, 1.0 + 3.0 * st.x);
    const vec3 vw = vec3(4.0 - 3.0 * st.y, 7.0, 1.0 + 3.0 * st.y);

    const vec3 u = vec3((3.0 - 2.0 * st.x) / uw.x - 2.0, (3.0 + st.x) / uw.y, st.x / uw.z + 2.0) * params.texel_size;
    const vec3 v = vec3((3.0 - 2.0 * st.y) / vw.x - 2.0, (3.0 + st.y) / vw.y, st.y / vw.z + 2.0) * params.texel_size;

    float sum = 0.0;
    sum += uw.x * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.x), proj.z), params, bias);
    sum += uw.y * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.x), proj.z), params, bias);
    sum += uw.z * vw.x * sample_shadow(shadow_map, vec3(base + vec2(u.z, v.x), proj.z), params, bias);
    sum += uw.x * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.y), proj.z), params, bias);
    sum += uw.y * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.y), proj.z), params, bias);
    sum += uw.z * vw.y * sample_shadow(shadow_map, vec3(base + vec2(u.z, v.y), proj.z), params, bias);
    sum += uw.x * vw.z * sample_shadow(shadow_map, vec3(base + vec2(u.x, v.z), proj.z), params, bias);
    sum += uw.y * vw.z * sample_shadow(shadow_map, vec3(base + vec2(u.y, v.z), proj.z), params, bias);
    sum += uw.z * vw.z * sample_shadow(shadow_map, vec3(base + vec2(u.z, v.z), proj.z), params, bias);
    return sum / 144.0;
#endif
}


float compute_shadow_hard(sampler2DShadow shadow_map, ShadowMapParams params, vec3 world_pos) {
    return compute_shadow_hard(shadow_map, params, world_pos, 0.0);
}

float compute_shadow_pcf(sampler2DShadow shadow_map, ShadowMapParams params, vec3 world_pos) {
    return compute_shadow_pcf(shadow_map, params, world_pos, 0.0);
}


#endif


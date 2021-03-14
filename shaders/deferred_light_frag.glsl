#include "deferred_light.glsl"

// -------------------------------- I/O --------------------------------
layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 1) readonly buffer Lights {
    Light lights[];
};

layout(set = 0, binding = 2) uniform sampler2D in_depth;
layout(set = 0, binding = 3) uniform sampler2D in_rt0;
layout(set = 0, binding = 4) uniform sampler2D in_rt1;

#ifdef SPOT_LIGHT
layout(set = 0, binding = 5) uniform sampler2DShadow in_shadows;
layout(set = 0, binding = 6) readonly buffer Shadows {
    ShadowMapParams shadow_params[];
};
#endif


layout(location = 0) flat in uint in_instance_id;

layout(location = 0) out vec4 out_color;



// -------------------------------- SHADOWS --------------------------------

float sample_shadow(sampler2DShadow shadow_map, vec2 uvs, float proj_z, float bias_scale) {
    const float bias = fwidth(proj_z) * bias_scale;
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

// -------------------------------- MAIN --------------------------------

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const vec2 uv = gl_FragCoord.xy / vec2(textureSize(in_depth, 0).xy);

    const GBufferData gbuffer = read_gbuffer(texelFetch(in_rt0, coord, 0), texelFetch(in_rt1, coord, 0));

    const float depth = texelFetch(in_depth, coord, 0).x;

    const vec3 world_pos = unproject(uv, depth, camera.inv_view_proj);
    const vec3 view_dir = normalize(camera.position - world_pos);

    vec3 irradiance = vec3(0.0);
    {
        const Light light = lights[in_instance_id];

        vec3 light_dir = light.position - world_pos;
        const float distance = length(light_dir);
        light_dir /= distance;
        float att = attenuation(distance, light.radius, light.falloff);

#ifdef SPOT_LIGHT
        const float spot_cos_alpha = -dot(light_dir, light.forward);
        att *= pow(max(0.0, (spot_cos_alpha - light.cos_angle) / (1.0 - light.cos_angle)), light.angle_exp);

        if(att > 0.0 && light.shadow_map_index < 0xFFFFFFFF) {
            const ShadowMapParams params = shadow_params[light.shadow_map_index];
            att *= compute_shadow_pcf(in_shadows, params, world_pos);
        }
#endif

        if(att > 0.0) {
            const vec3 radiance = light.color * att;
            irradiance += radiance * L0(light_dir, view_dir, gbuffer);
        }
    }

    out_color = vec4(irradiance, 1.0);
}


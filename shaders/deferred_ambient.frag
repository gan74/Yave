#version 450

#include "lib/lighting.glsl"
#include "lib/gbuffer.glsl"
#include "lib/ibl.glsl"
#include "lib/shadow.glsl"

// -------------------------------- DEFINES --------------------------------

#define USE_IBL
#define USE_AO

// #define DEBUG_CASCADES

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_rt0;
layout(set = 0, binding = 2) uniform sampler2D in_rt1;
layout(set = 0, binding = 3) uniform sampler2DShadow in_shadows;
layout(set = 0, binding = 4) uniform sampler2D in_ao;

layout(set = 0, binding = 5) uniform samplerCube in_envmap;
layout(set = 0, binding = 6) uniform sampler2D brdf_lut;

layout(set = 0, binding = 7) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 8) readonly buffer Lights {
    DirectionalLight lights[];
};

layout(set = 0, binding = 9) readonly buffer Shadows {
    ShadowMapParams shadow_params[];
};

layout(set = 0, binding = 10) uniform Params {
    uint light_count;
    uint display_sky;
    float ibl_intensity;
    float padding_;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec3 out_color;


// -------------------------------- HELPERS --------------------------------

float ambient_occlusion() {
#ifdef USE_AO
    return texture(in_ao, in_uv).r;
#else
    return 1.0;
#endif
}


uint shadow_cascade_index(uvec4 indices, vec3 world_pos) {
    for(uint i = 0; i != 4; ++i) {
        const uint index = indices[i];
        if(index >= 0xFFFFFFFF) {
            break;
        }

        const ShadowMapParams params = shadow_params[index];
        const vec2 coords = abs(project(world_pos, params.view_proj).xy * 2.0 - 1.0);
        if(max(coords.x, coords.y) < 1.0 - (params.texel_size * 4.0)) {
            return index;
        }
    }

    return 0xFFFFFFFF;
}

vec3 cascade_debug_color(uint shadow_map_index, uvec4 indices) {
    if(shadow_map_index == indices[0]) {
        return vec3(0.0, 0.0, 1.0);
    }
    if(shadow_map_index == indices[1]) {
        return vec3(0.0, 1.0, 0.0);
    }
    if(shadow_map_index == indices[2]) {
        return vec3(1.0, 1.0, 0.0);
    }
    return vec3(1.0, 0.0, 0.0);
}

vec4 cascade_debug_color(vec3 world_pos) {
    if(light_count > 0) {
        const uvec4 indices = lights[0].shadow_map_indices;
        const uint shadow_map_index = shadow_cascade_index(indices, world_pos);

        if(shadow_map_index < 0xFFFFFFFF) {
            const ShadowMapParams params = shadow_params[shadow_map_index];
            const vec3 proj = project(world_pos, params.view_proj);
            const vec2 texel = floor(atlas_uv(params, proj.xy) * params.size);
            const vec3 color = mix(uv_debug_color(texel), cascade_debug_color(shadow_map_index, indices), 0.25);
            return vec4(color, 1.0);
        }
    }
    return vec4(0.0);
}



// -------------------------------- MAIN --------------------------------

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const float depth = texelFetch(in_depth, coord, 0).x;
    vec3 irradiance = vec3(0.0);

    if(is_OOB(depth)) {
        const vec3 view_dir = -view_direction(camera, in_uv);

#if defined(USE_IBL)
        if(display_sky != 0) {
            irradiance = texture(in_envmap, view_dir).rgb;
        }
#endif

        for(uint i = 0; i != light_count; ++i) {
            const DirectionalLight light = lights[i];
            if(dot(view_dir, light.direction) > light.cos_disk) {
                irradiance = light.color;
            }
        }
    } else {
        const SurfaceInfo surface = read_gbuffer(texelFetch(in_rt0, coord, 0), texelFetch(in_rt1, coord, 0));

        const vec3 world_pos = unproject(in_uv, depth, camera.inv_view_proj);
        const vec3 view_dir = normalize(camera.position - world_pos);

        // directional lights
        for(uint i = 0; i != light_count; ++i) {
            const DirectionalLight light = lights[i];
            const vec3 light_dir = light.direction; // assume normalized

            float att = 1.0;

            const uint shadow_map_index = shadow_cascade_index(light.shadow_map_indices, world_pos);
            if(shadow_map_index < 0xFFFFFFFF) {
                const ShadowMapParams params = shadow_params[shadow_map_index];
                att = compute_shadow_pcf(in_shadows, params, world_pos);
            }

            if(att > 0.0) {
                const vec3 radiance = light.color * att;
                irradiance += radiance * eval_lighting(surface, view_dir, light_dir);
            }
        }

#ifdef USE_IBL
        irradiance += eval_ibl(in_envmap, brdf_lut, view_dir, surface) * ambient_occlusion() * ibl_intensity;
#endif

#if defined(DEBUG_CASCADES)
        const vec4 debug_color = cascade_debug_color(world_pos);
        irradiance = mix(irradiance, debug_color.rgb, debug_color.a);
#endif
    }

    out_color = irradiance;
}


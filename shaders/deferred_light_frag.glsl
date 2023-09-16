#include "deferred_light.glsl"

#include "lib/shadow.glsl"


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


// -------------------------------- MAIN --------------------------------

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const vec2 uv = gl_FragCoord.xy / vec2(textureSize(in_depth, 0).xy);

    const float depth = texelFetch(in_depth, coord, 0).x;

    const SurfaceInfo surface = read_gbuffer(texelFetch(in_rt0, coord, 0), texelFetch(in_rt1, coord, 0));

    const vec3 world_pos = unproject(uv, depth, camera.inv_view_proj);
    const vec3 view_dir = normalize(camera.position - world_pos);

    vec3 irradiance = vec3(0.0);
    {
        const Light light = lights[in_instance_id];
        const AreaLightInfo area = karis_area_light(surface, light, world_pos, view_dir);
        float att = attenuation(area.orig_light_dist * light.falloff, light.range * light.falloff, light.min_radius * light.falloff);

#ifdef SPOT_LIGHT
        if(att > 0.0) {
            const float spot_cos_alpha = max(0.0, -dot(area.light_dir, light.forward));
            att *= spot_attenuation(spot_cos_alpha, light.att_scale_offset);

            if(att > 0.0 && light.shadow_map_index < 0xFFFFFFFF) {
                const ShadowMapParams params = shadow_params[light.shadow_map_index];
                att *= compute_shadow_pcf(in_shadows, params, world_pos);
            }
        }
#endif

        if(att > 0.0) {
            const vec3 radiance = light.color * att;
            irradiance += radiance * L0(area.light_dir, view_dir, alpha_corrected(surface, area));
        }
    }

    out_color = vec4(irradiance, 1.0);
}


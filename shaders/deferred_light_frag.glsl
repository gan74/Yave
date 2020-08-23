#include "deferred_light.glsl"

#include "lib/lighting.glsl"
#include "lib/gbuffer.glsl"
#include "lib/shadows.glsl"
#include "lib/ibl.glsl"


// -------------------------------- I/O --------------------------------
layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 1) readonly buffer Lights {
    Light lights[];
};

layout(set = 0, binding = 2) uniform sampler2D in_depth;
layout(set = 0, binding = 3) uniform sampler2D in_color;
layout(set = 0, binding = 4) uniform sampler2D in_normal;

#ifdef SPOT_LIGHT
layout(set = 0, binding = 5) uniform sampler2D in_shadows;
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

    vec3 albedo;
    float metallic;
    vec3 normal;
    float roughness;
    unpack_color(texelFetch(in_color, coord, 0), albedo, metallic);
    unpack_normal(texelFetch(in_normal, coord, 0), normal, roughness);

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
            att *= compute_shadow(in_shadows, params, world_pos);
        }
#endif

        if(att > 0.0) {
            const vec3 radiance = light.color * att;
            irradiance += radiance * L0(normal, light_dir, view_dir, roughness, metallic, albedo);
        }
    }

    out_color = vec4(irradiance, 1.0);
}


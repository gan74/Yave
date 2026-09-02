#include "lib/utils.slang"
#include "lib/atmosphere.slang"

[[vk::binding(0)]]
Sampler2D<float> in_depth;

[[vk::binding(1)]]
Sampler2D in_color;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
Sampler2D transmittance_texture;

[[vk::binding(4)]]
Sampler3D scattering_texture;

[[vk::binding(5)]]
ConstantBuffer<AtmosphereParams> atmos_params;

[[vk::binding(6)]]
cbuffer Params {
    float3 sun_direction;
    float sun_intensity;
    float3 sun_color;
    float sea_level;
};


float3 view_direction(float2 uv) {
    return normalize(unproject(uv, 0.001, camera.curr.inv_view_proj) - camera.position);
}

float3 planet_space(float3 world_pos) {
    return world_pos - float3(0.0, sea_level - atmos_params.bottom_radius, 0.0);
}

float3 clamp_camera_to_atmosphere(float3 camera_planet) {
    const float r = length(camera_planet);
    const float min_r = atmos_params.bottom_radius + 1.0;
    if(r < min_r) {
        return camera_planet * (min_r / max(r, 1.0));
    }
    return camera_planet;
}

[shader("fragment")]
float4 frag_main(float2 in_uv) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);
    const float depth = in_depth[coord];
    const float3 color = in_color[coord].rgb;

    const float3 sun_dir = normalize(sun_direction);
    const float3 camera_planet = clamp_camera_to_atmosphere(planet_space(camera.position));

    float3 transmittance;
    if(is_depth_far(depth)) {
        const float3 view_dir = view_direction(in_uv);
        float3 radiance = sky_radiance(
            atmos_params,
            transmittance_texture,
            scattering_texture,
            camera_planet,
            view_dir,
            sun_dir,
            transmittance);

        if(dot(view_dir, sun_dir) > cos(atmos_params.sun_angular_radius)) {
            radiance += transmittance * atmos_params.solar_irradiance;
        }

        return float4(radiance * sun_color * sun_intensity, 1.0);
    }

    const float3 world_pos = unproject(in_uv, depth, camera.curr.inv_view_proj);
    float3 point_planet = planet_space(world_pos);

    {
        const float r = length(point_planet);
        const float min_r = atmos_params.bottom_radius + 1.0;
        if(r < min_r) {
            point_planet *= min_r / max(r, 1.0);
        }
    }

    const float3 in_scatter = sky_radiance_to_point(
        atmos_params,
        transmittance_texture,
        scattering_texture,
        camera_planet,
        point_planet,
        sun_dir,
        transmittance);

    return float4(color * transmittance + in_scatter * sun_color * sun_intensity, 1.0);
}

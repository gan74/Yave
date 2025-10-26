#include "lib/utils.slang"
#include "lib/brdf.slang"

#ifdef POINT
typealias Light = PointLight;

#endif

#ifdef SPOT
typealias Light = SpotLight;
#endif



[[vk::binding(0)]]
ConstantBuffer<Camera> camera;

[[vk::binding(1)]]
StructuredBuffer<Light> lights;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D in_rt0;

[[vk::binding(4)]]
Sampler2D<uint> in_rt1;

#ifdef SPOT
[[vk::binding(5)]]
Sampler2DShadow in_shadows;

[[vk::binding(6)]]
StructuredBuffer<ShadowMapInfo> shadow_infos;
#endif





struct VertexStageOut {
    float4 sv_position  : SV_Position;
    uint instance_index;
};


[shader("vertex")]
VertexStageOut vert_main(float3 position) {
    const Light light = lights[semantics.instance_index];

    VertexStageOut out;
    {

        out.instance_index = semantics.instance_index;

#ifdef SPOT
        out.sv_position = mul(camera.curr.view_proj, mul(light.draw_model, float4(position, 1.0)));
#else
        const float scale = light.range * 1.1;
        out.sv_position = mul(camera.curr.view_proj, float4(position * scale + light.position, 1.0));
#endif

    }
    return out;
}


[shader("fragment")]
float4 frag_main(uint in_instance_index) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);
    const float2 uv = semantics.frag_coord.xy / float2(image_size(in_depth).xy);

    const float depth = in_depth[coord];

    const SurfaceInfo surface = decode_gbuffer(RawGBuffer(
        in_rt0[coord],
        in_rt1[coord],
    ));

    const float3 world_pos = unproject(uv, depth, camera.curr.inv_view_proj);
    const float3 view_dir = normalize(camera.position - world_pos);

    float3 irradiance = float3(0.0);
    {
        const Light light = lights[in_instance_index];
        const AreaLightInfo area = karis_area_light(surface, light, world_pos, view_dir);
        float att = attenuation(area.orig_light_dist * light.falloff, light.range * light.falloff, light.min_radius * light.falloff);

#ifdef SPOT
        if(att > 0.0) {
            const float spot_cos_alpha = max(0.0, -dot(area.light_dir, light.forward));
            att *= spot_attenuation(spot_cos_alpha, light.att_scale_offset);

            if(att > 0.0 && light.shadow_map_index < 0xFFFFFFFF) {
                const ShadowMapInfo sm_info = shadow_infos[light.shadow_map_index];
                att *= compute_shadow_pcf(in_shadows, sm_info, world_pos);
            }
        }
#endif

        if(att > 0.0) {
            const float3 radiance = light.color * att;
            irradiance += radiance * eval_brdf(alpha_corrected_surface(surface, area), view_dir, area.light_dir);
        }
    }

    return float4(irradiance, 1.0);
}


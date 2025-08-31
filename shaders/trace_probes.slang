#include "lib/gi.slang"
#include "lib/rt.slang"

[[vk::binding(0)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(1)]]
ConstantBuffer<Camera> camera;

[[vk::binding(2)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(3)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(4)]]
SamplerCube in_envmap;

[[vk::binding(5)]]
RWTexture2D out_image;

[[vk::binding(0, 1)]]
Sampler2D all_textures[];




float4 eval_material_color(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    return all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
}

[shader("compute")]
[numthreads(gi.probe_size, gi.probe_size)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint probe_index = semantics.global_id.z;

    const float3 probe_pos = gi.compute_world_coord(gi.compute_grid_coord(probe_index), camera.position);

    const float2 px_center = float2(coord) + 0.5;
    const float2 uv = px_center / gi.probe_size;

    const float3 trace_dir = octahedron_decode(uv);

    const float tmin = 0.01;
    const float tmax = 10000.0; // <-------------------------------------------------------------------- TODO
    const HitInfo hit = trace_inline(tlas, probe_pos, trace_dir, tmin, tmax);

    float3 color = float3(0.0);
    if(hit.hit) {
#if 1
        const DirectionalLight light = lights[0];
        if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
            const float3 normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
            const float NoL = dot(light.direction, normal);
            if(NoL > 0.0) {
                const float4 albedo = eval_material_color(hit.instance_id);
                color = albedo.rgb * light.color * NoL;
            }
        }
#else
        color = eval_material_color(hit.instance_id).rgb;
#endif
    } else {
        color = in_envmap.SampleLevel(trace_dir, 0.0).rgb;
    }

    out_image[coord + gi.compute_atlas_coord(probe_index) * gi.probe_size] = float4(color, hit.ray_t);
}

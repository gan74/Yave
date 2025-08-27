#include "lib/utils.slang"
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
RWTexture2D out_image;

[[vk::binding(0, 1)]]
Sampler2D all_textures[];




float4 eval_material_color(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    return all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
}


int3 compute_coord(uint instance_index) {
    const int3 coord = uint3(
        (instance_index / (32 * 32)) % 32,
        (instance_index / 32) % 32,
        instance_index % 32
    );
    return coord;
}


[shader("compute")]
[numthreads(32, 32)]
void comp() {
    const uint2 coord = semantics.global_id.xy;
    const uint probe_index = semantics.global_id.z;

    const int3 probe_coord = compute_coord(probe_index);

    const float2 px_center = float2(coord) + float2(0.5);
    const float2 uv = px_center / semantics.work_group_size.xy;

    const float3 trace_dir = octahedron_decode(uv);
    const float3 probe_pos = float3(int3(camera.position) + probe_coord - int3(15));

    const float tmin = 0.01;
    const float tmax = 10000.0;
    const HitInfo hit = trace_inline(tlas, probe_pos, trace_dir, tmin, tmax);

    float3 color = float3(0.0);
    if(hit.hit) {
        const DirectionalLight light = lights[0];
        if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
            const float3 normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
            const float NoL = dot(light.direction, normal);
            if(NoL > 0.0) {
                const float4 albedo = eval_material_color(hit.instance_id);
                color = albedo.rgb * light.color * NoL;
            }
        }
    }

    out_image[coord + uint2(probe_index / 256, probe_index % 256) * semantics.work_group_size.xy] = float4(color, 1.0);
}

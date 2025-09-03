#include "lib/ddgi.slang"
#include "lib/rt.slang"


[[vk::binding(0)]]
RWStructuredBuffer<DDGIRayHit> out_hits;

[[vk::binding(1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(2)]]
ConstantBuffer<Camera> camera;

[[vk::binding(3)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(4)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(5)]]
SamplerCube in_envmap;

[[vk::binding(0, 1)]]
Sampler2D all_textures[];


float3 sample_sphere_uniform(uint i, uint count) {
    const float theta = 2.0 * pi * i * golden;
    const float phi = acos(1.0 - 2.0 * i / float(count));
    return float3(float2(cos(theta), sin(theta)) * sin(phi), cos(phi));
}

float4 eval_material_color(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    return all_textures[albedo_id].SampleLevel(float2(0.5), 100.0);
}


[shader("compute")]
[numthreads(64, 1)]
void comp() {
    const uint probe_index = semantics.global_id.x / ddgi.probe_rays;
    const uint ray_index = semantics.global_id.x % ddgi.probe_rays;
    const int3 probe_id = ddgi.compute_grid_coord(probe_index);
    const float3 probe_pos = ddgi.compute_world_coord(probe_id, camera.position);

    const float3 trace_dir = sample_sphere_uniform(ray_index, ddgi.probe_rays);

    const float tmin = 0.01;
    const float tmax = 10000.0; // <-------------------------------------------------------------------- TODO
    const HitInfo hit = trace_inline(tlas, probe_pos, trace_dir, tmin, tmax);
    float3 hit_normal = float3(0.0);

    float3 color = float3(0.0);
    if(hit.hit) {
        const DirectionalLight light = lights[0];
        hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
        const float NoL = dot(light.direction, hit_normal);
        const bool cast_shadow = light.cast_shadow != 0;
        if(NoL > 0.0 && (!cast_shadow || !trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit)) {
            const float4 albedo = eval_material_color(hit.instance_id);
            color = albedo.rgb * light.color * NoL;
        }
    } else {
        color = in_envmap.SampleLevel(trace_dir, 0.0).rgb;
    }

    DDGIRayHit out_hit;
    {
        out_hit.dir = trace_dir;
        out_hit.t = hit.ray_t;
        out_hit.normal = hit_normal;
        out_hit.is_hit = hit.hit ? 1 : 0;
        out_hit.radiance = color;
        out_hit.padding = 0.0;
    }

    out_hits[semantics.global_id.x] = out_hit;
}

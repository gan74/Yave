#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D out_radiance;

[[vk::binding(1)]]
RWTexture2D<float2> out_distance;

[[vk::binding(2)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(3)]]
SamplerCube in_envmap;

[[vk::binding(4)]]
StructuredBuffer<MaterialData> materials;

[[vk::binding(5)]]
StructuredBuffer<DirectionalLight> lights;

[[vk::binding(6)]]
cbuffer Params {
    float probe_spacing;
    uint light_count;
    uint frame_id;
    float _pad;
};


[[vk::binding(0, 1)]]
Sampler2D all_textures[];



struct DDGIMaterial {
    float3 albedo;
    float3 emissive;
};

DDGIMaterial eval_material(uint instance_id) {
    const MaterialData material = materials[instance_id];
    const uint albedo_id = material.texture_indices[uint(TextureSlots::Albedo)];
    const uint emissive_id = material.texture_indices[uint(TextureSlots::Emissive)];
    return DDGIMaterial(
        all_textures[NonUniformResourceIndex(albedo_id)].SampleLevel(float2(0.5), 100.0).rgb * material.base_color_factor,
        all_textures[NonUniformResourceIndex(emissive_id)].SampleLevel(float2(0.5), 100.0).rgb * material.emissive_factor,
    );
}

float3 shade_hit(HitInfo hit, float3 trace_dir) {
    if(!hit.hit) {
        return in_envmap.SampleLevel(trace_dir, 0.0).rgb;
    }

    const float3 hit_normal = normalize(mul(float3x3(hit.model_to_world), hit.ms_normal));
    const DDGIMaterial material = eval_material(hit.instance_id);
    const float n_t = constants.inv_pi;

    float3 radiance = material.albedo * in_envmap.SampleLevel(hit_normal, 999.0).rgb * n_t;

    const float tmin = 0.001;
    const float tmax = 10000.0;
    for(uint k = 0; k != light_count; ++k) {
        const DirectionalLight light = lights[k];
        const float n_l = dot(light.direction, hit_normal);
        if(n_l > 0.0) {
            if(!trace_inline(tlas, hit.position, light.direction, tmin, tmax).hit) {
                radiance += material.albedo * light.color * n_l * n_t;
            }
        }
    }

    return radiance;
}


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    if(any(coord >= ddgi.radiance_atlas_size)) {
        return;
    }

    const uint2 probe_local = coord % ddgi.radiance_probe_size;
    const uint2 probe_grid = coord / ddgi.radiance_probe_size;
    const uint probe_index = probe_grid.y * ddgi.probes_per_atlas_row + probe_grid.x;
    if(probe_index >= ddgi.probe_count) {
        return;
    }

    const int3 grid_coord = ddgi.compute_grid_coord(probe_index);
    const float3 origin = ddgi.probe_world_position(grid_coord, probe_spacing);

    const float2 octa_uv = (float2(probe_local) + 0.5) / float(ddgi.radiance_probe_size);
    const float3 trace_dir = octahedron_decode(octa_uv);

    const float tmin = 0.01;
    const float tmax = 10000.0;
    const HitInfo hit = trace_inline(tlas, origin, trace_dir, tmin, tmax);

    out_radiance[coord] = float4(shade_hit(hit, trace_dir), 1.0);
    out_distance[coord] = float2(hit.ray_t, sqr(hit.ray_t));
}

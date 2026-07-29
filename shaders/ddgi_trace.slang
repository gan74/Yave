#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/rt.slang"
#include "lib/rng.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D<float3> out_radiance;

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
Sampler2D<uint> probe_grid;

[[vk::binding(7)]]
cbuffer Params {
    float probe_spacing;
    uint light_count;
    uint frame_id;
    uint reset;
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
[numthreads(64, 1, 1)]
void main() {
    const uint grid_probe_index = semantics.global_id.x;
    if(grid_probe_index >= ddgi.grid_cell_count) {
        return;
    }

    const int3 grid_coord = ddgi.compute_grid_coord(grid_probe_index);
    const uint atlas_probe_index = ddgi.load_probe_index(probe_grid, grid_coord);

    if(atlas_probe_index == ddgi.invalid_probe_index) {
        return;
    }

#ifdef BORDER
    const uint border_index = semantics.global_id.y;
    if(border_index >= ddgi.probe_border_texel_count(ddgi.radiance_probe_size)) {
        return;
    }

    const uint2 probe_local = ddgi.probe_border_local(border_index, ddgi.radiance_probe_size);

    const uint2 probe_corner = ddgi.radiance_atlas_coord(atlas_probe_index);
    const uint2 source_local = ddgi.probe_border_source_local(probe_local, ddgi.radiance_probe_size);
    const uint2 source_coord = probe_corner + source_local;
    const uint2 dest_coord = probe_corner + probe_local;

    out_radiance[dest_coord] = out_radiance[source_coord];
    out_distance[dest_coord] = out_distance[source_coord];
#else

    const uint2 probe_local = semantics.global_id.yz;
    const uint2 coord = ddgi.radiance_atlas_coord(atlas_probe_index) + ddgi.probe_border + probe_local;

    const float3 origin = ddgi.probe_world_position(grid_coord, probe_spacing);

    const uint seed = hash.xorshift(semantics.global_index + hash.xorshift(frame_id));
    HashRng rng = HashRng(seed);

    float3 radiance = 0.0;
    float2 distances = 0.0;

    const uint sample_count = 1;
    for(uint i = 0; i != sample_count; ++i) {
        const float2 octa_uv = (float2(probe_local) + float2(rng.next_unorm(), rng.next_unorm())) / float(ddgi.radiance_probe_data_size);
        const float3 trace_dir = octahedron_decode(octa_uv);

        const float tmin = 0.01;
        const float tmax = 10000.0;
        const HitInfo hit = trace_inline(tlas, origin, trace_dir, tmin, tmax);

        if(!hit.front_face) {
            continue;
        }

        const float dist = saturate(hit.ray_t / (probe_spacing * constants.sqrt_3));

        radiance += shade_hit(hit, trace_dir);
        distances += float2(dist, sqr(dist));
    }

    if(reset != 0) {
        out_radiance[coord] = radiance / sample_count;
        out_distance[coord] = distances / sample_count;
    } else {
        const float blend_factor = 0.9;
        out_radiance[coord] = lerp(
            radiance / sample_count,
            out_radiance[coord].rgb,
            blend_factor
        );
        // out_distance[coord] = lerp(distances / sample_count, out_distance[coord], blend_factor);
        out_distance[coord] = max(distances / sample_count, out_distance[coord]);
    }

    // out_radiance[coord] = identifying_color(grid_probe_index);

#endif
}

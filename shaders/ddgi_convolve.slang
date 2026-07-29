#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D<float3> out_irradiance;

[[vk::binding(1)]]
Sampler2D<float3> in_radiance;

[[vk::binding(2)]]
Sampler2D<uint> probe_grid;

[[vk::binding(3)]]
cbuffer Params {
    uint sample_count;
    uint atlas_probe_count;
};



float3 cosine_sample_hemisphere(float3 normal, float2 xi) {
    const float phi = 2.0 * constants.pi * xi.x;
    const float cos_theta = sqrt(1.0 - xi.y);
    const float sin_theta = sqrt(xi.y);

    const float3 local = float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    const float3 up = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    const float3 tangent = normalize(cross(up, normal));
    const float3 bitangent = cross(normal, tangent);

    return normalize(tangent * local.x + bitangent * local.y + normal * local.z);
}

float3 sample_probe_radiance(uint atlas_probe_index, float3 dir) {
    const uint2 probe_corner = ddgi.radiance_atlas_coord(atlas_probe_index) + ddgi.probe_border;
    const float2 sample_coord = octahedron_encode(dir) * float(ddgi.radiance_probe_data_size - 1);
    const float2 uv = (float2(probe_corner) + sample_coord + 0.5) / float2(ddgi.radiance_atlas_size(atlas_probe_count));
    return in_radiance.SampleLevel(uv, 0.0);
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
    if(border_index >= ddgi.probe_border_texel_count(ddgi.irradiance_probe_size)) {
        return;
    }

    const uint2 probe_local = ddgi.probe_border_local(border_index, ddgi.irradiance_probe_size);

    const uint2 probe_corner = ddgi.irradiance_atlas_coord(atlas_probe_index);
    const uint2 source_local = ddgi.probe_border_source_local(probe_local, ddgi.irradiance_probe_size);
    const uint2 source_coord = probe_corner + source_local;
    const uint2 dest_coord = probe_corner + probe_local;

    out_irradiance[dest_coord] = out_irradiance[source_coord];
#else

    const uint2 probe_local = semantics.global_id.yz;
    const uint2 coord = ddgi.irradiance_atlas_coord(atlas_probe_index) + ddgi.probe_border + probe_local;

    const float2 octa_uv = (float2(probe_local) + 0.5) / float(ddgi.irradiance_probe_data_size);
    const float3 normal = octahedron_decode(octa_uv);

    // Cosine-weighted hemisphere samples of probe radiance.
    // With PDF = cos(θ)/π, the estimator for E/π is just the average of L.
    float3 acc = float3(0.0);
    for(uint i = 0; i != sample_count; ++i) {
        const float2 xi = hammersley(i, sample_count);
        const float3 dir = cosine_sample_hemisphere(normal, xi);
        acc += sample_probe_radiance(atlas_probe_index, dir);
    }

    out_irradiance[coord] = acc / float(sample_count);
#endif
}

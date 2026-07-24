#include "lib/semantics.slang"
#include "lib/utils.slang"
#include "lib/ddgi.slang"


[[vk::binding(0)]]
RWTexture2D out_irradiance;

[[vk::binding(1)]]
Sampler2D<float3> in_radiance;

[[vk::binding(2)]]
cbuffer Params {
    uint sample_count;
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

float3 sample_probe_radiance(uint probe_index, float3 dir) {
    const uint2 probe_corner = ddgi.radiance_atlas_coord(probe_index);
    const float2 sample_coord = octahedron_encode(dir) * float(ddgi.radiance_probe_size - 1);
    const float2 uv = (float2(probe_corner) + sample_coord + 0.5) / float2(ddgi.radiance_atlas_size);
    return in_radiance.SampleLevel(uv, 0.0);
}


[shader("compute")]
[numthreads(8, 8)]
void main() {
    const uint2 coord = semantics.global_id.xy;
    if(any(coord >= ddgi.irradiance_atlas_size)) {
        return;
    }

    const uint2 probe_local = coord % ddgi.irradiance_probe_size;
    const uint2 probe_grid = coord / ddgi.irradiance_probe_size;
    const uint probe_index = probe_grid.y * ddgi.probes_per_atlas_row + probe_grid.x;
    if(probe_index >= ddgi.probe_count) {
        return;
    }

    const float2 octa_uv = (float2(probe_local) + 0.5) / float(ddgi.irradiance_probe_size);
    const float3 normal = octahedron_decode(octa_uv);

    // Cosine-weighted hemisphere samples of probe radiance.
    // With PDF = cos(θ)/π, the estimator for E/π is just the average of L.
    float3 acc = float3(0.0);
    for(uint i = 0; i != sample_count; ++i) {
        const float2 xi = hammersley(i, sample_count);
        const float3 dir = cosine_sample_hemisphere(normal, xi);
        acc += sample_probe_radiance(probe_index, dir);
    }

    out_irradiance[coord] = float4(acc / float(sample_count), 1.0);
}

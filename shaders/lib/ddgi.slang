#ifndef DDGI_SLANG
#define DDGI_SLANG

#include "utils.slang"


namespace ddgi {

static const uint grid_size = 32;
static const uint radiance_probe_size = 32;
static const uint irradiance_probe_size = radiance_probe_size / 2;
static const uint probes_per_atlas_row = 256;

static const uint probe_count = grid_size * grid_size * grid_size;

static const uint2 radiance_atlas_size = uint2(
    probes_per_atlas_row * radiance_probe_size,
    ((probe_count + probes_per_atlas_row - 1) / probes_per_atlas_row) * radiance_probe_size
);

static const uint2 irradiance_atlas_size = uint2(
    probes_per_atlas_row * irradiance_probe_size,
    ((probe_count + probes_per_atlas_row - 1) / probes_per_atlas_row) * irradiance_probe_size
);


uint compute_probe_index(int3 grid_coord) {
    return uint(grid_coord.x) * (grid_size * grid_size) + uint(grid_coord.y) * grid_size + uint(grid_coord.z);
}

int3 compute_grid_coord(uint probe_index) {
    return int3(
        int((probe_index / (grid_size * grid_size)) % grid_size),
        int((probe_index / grid_size) % grid_size),
        int(probe_index % grid_size)
    );
}

int3 compute_grid_coord(float3 world_pos, float probe_spacing) {
    const float3 grid = world_pos / probe_spacing + (grid_size - 1) * 0.5;
    return clamp(int3(round(grid)), int3(0), int3(grid_size - 1));
}

int3 compute_grid_coord_low(float3 world_pos, float probe_spacing) {
    const float3 grid = world_pos / probe_spacing + (grid_size - 1) * 0.5;
    return clamp(int3(floor(grid)), int3(0), int3(grid_size - 1));
}

int3 compute_grid_coord_high(float3 world_pos, float probe_spacing) {
    const float3 grid = world_pos / probe_spacing + (grid_size - 1) * 0.5;
    return clamp(int3(ceil(grid)), int3(0), int3(grid_size - 1));
}

float3 probe_world_position(int3 grid_coord, float probe_spacing) {
    return (float3(grid_coord) - (grid_size - 1) * 0.5) * probe_spacing;
}

uint2 radiance_atlas_coord(uint probe_index) {
    return uint2(probe_index % probes_per_atlas_row, probe_index / probes_per_atlas_row) * radiance_probe_size;
}

uint2 irradiance_atlas_coord(uint probe_index) {
    return uint2(probe_index % probes_per_atlas_row, probe_index / probes_per_atlas_row) * irradiance_probe_size;
}

bool in_range(float3 world_pos, float probe_spacing) {
    const float3 extent = (grid_size - 1) * 0.5 * probe_spacing;
    return all(abs(world_pos) <= extent + probe_spacing);
}

}

struct IrradianceFieldSampleParams {
    float sampling_normal_bias;
    float sampling_view_dir_bias;
    float distance_bias;
    float variance_bias;
    float chebyshev_bias;

    __init() {
        sampling_normal_bias = 0.2;
        sampling_view_dir_bias = 0.2;
        distance_bias = 0.0;
        variance_bias = 1.0e-4;
        chebyshev_bias = 0.0;
    }
};


float3 encode_irr(float3 irr) {
    return sqrt(max(irr, 0.0));
}

float3 decode_irr(float3 irr) {
    return sqr(irr);
}


float3 sample_irradiance(
    float3 world_pos,
    float3 normal,
    float3 cam_pos,
    float probe_spacing,
    Sampler2D<float3> irradiance_tex,
    Sampler2D<float2> distance_tex,
    IrradianceFieldSampleParams params = IrradianceFieldSampleParams()) {

    if(!ddgi.in_range(world_pos, probe_spacing)) {
        return float3(0.0);
    }

    const float2 inv_radiance_atlas_size = 1.0 / float2(ddgi.radiance_atlas_size);
    const float2 inv_irradiance_atlas_size = 1.0 / float2(ddgi.irradiance_atlas_size);
    const float2 irr_sample_coord = octahedron_encode(normal) * float(ddgi.irradiance_probe_size - 1);

    const float3 view_dir = normalize(cam_pos - world_pos);
    const float3 bias = normal * params.sampling_normal_bias + view_dir * params.sampling_view_dir_bias;
    const float3 biased_world_pos = world_pos + bias * probe_spacing;

    float total = 0.0;
    float3 acc = 0.0;

    const int3 lo = ddgi.compute_grid_coord_low(biased_world_pos, probe_spacing);
    const int3 hi = ddgi.compute_grid_coord_high(biased_world_pos, probe_spacing);
    for(uint k = 0; k != 8; ++k) {
        const int3 probe_id = int3(
            ((k & 0x01) != 0) ? hi.x : lo.x,
            ((k & 0x02) != 0) ? hi.y : lo.y,
            ((k & 0x04) != 0) ? hi.z : lo.z
        );

        const float3 probe_pos = ddgi.probe_world_position(probe_id, probe_spacing);
        const float3 probe_vec = probe_pos - biased_world_pos;
        const float probe_dist = length(probe_vec);
        const uint probe_index = ddgi.compute_probe_index(probe_id);

        float weight = 1.0;

        {
            const uint2 dist_corner = ddgi.radiance_atlas_coord(probe_index);
            const float2 dist_sample_coord = octahedron_encode(probe_vec) * float(ddgi.radiance_probe_size - 1);
            const float2 dist_sample_uv = (float2(dist_corner) + dist_sample_coord + 0.5) * inv_radiance_atlas_size;

            const float2 dist_moments = distance_tex.SampleLevel(dist_sample_uv, 0.0);
            if(all(dist_moments == 0.0)) {
                continue;
            }

            const float mean = dist_moments.x + params.distance_bias;
            const float variance = abs(sqr(mean) - dist_moments.y) + params.variance_bias;

            const float chebyshev = variance / (variance + sqr(max(0.0, probe_dist - mean)));
            const float chebyshev_weight = max(0.0, sqr(chebyshev) - params.chebyshev_bias) / max(1.0 - params.chebyshev_bias, constants.epsilon);
            weight *= (probe_dist <= mean) ? 1.0 : chebyshev_weight;
        }

        {
            const float3 true_probe_vec = probe_pos - world_pos;
            const float true_dist = length(true_probe_vec);
            if(true_dist > constants.epsilon) {
                weight *= max(0.0, dot(true_probe_vec / true_dist, normal));
            }
        }

        const float crush_threshold = 0.2;
        if(weight < crush_threshold) {
            weight *= weight * weight * (1.0 / sqr(crush_threshold));
            weight = max(constants.epsilon, weight);
        }

        {
            const float3 trilinear = saturate(1.0 - abs(probe_vec) / probe_spacing);
            weight *= trilinear.x * trilinear.y * trilinear.z;
        }

        const uint2 irr_corner = ddgi.irradiance_atlas_coord(probe_index);
        const float2 sample_uv = (float2(irr_corner) + irr_sample_coord + 0.5) * inv_irradiance_atlas_size;
        const float3 irr = irradiance_tex.SampleLevel(sample_uv, 0.0);

        total += weight;
        acc += encode_irr(irr) * weight;
    }

    return total > constants.epsilon ? decode_irr(acc / total) : float3(0.0);
}


#endif // DDGI_SLANG

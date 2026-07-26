#ifndef DDGI_SLANG
#define DDGI_SLANG

#include "utils.slang"


namespace ddgi {

static const uint grid_size = 16;
static const uint radiance_probe_size = 32;
static const uint irradiance_probe_size = radiance_probe_size / 2;
static const uint probes_per_atlas_row = 256;

static const uint probes_per_cascade = grid_size * grid_size * grid_size;

// Per-cascade grid indexing.
static const uint probe_count = probes_per_cascade;


uint total_probe_count(uint cascade_count) {
    return probes_per_cascade * cascade_count;
}

uint2 radiance_atlas_size(uint cascade_count) {
    const uint count = total_probe_count(cascade_count);
    return uint2(
        probes_per_atlas_row * radiance_probe_size,
        ((count + probes_per_atlas_row - 1) / probes_per_atlas_row) * radiance_probe_size
    );
}

uint2 irradiance_atlas_size(uint cascade_count) {
    const uint count = total_probe_count(cascade_count);
    return uint2(
        probes_per_atlas_row * irradiance_probe_size,
        ((count + probes_per_atlas_row - 1) / probes_per_atlas_row) * irradiance_probe_size
    );
}

float cascade_probe_spacing(uint cascade, float base_spacing, float spacing_scale) {
    return base_spacing * pow(spacing_scale, float(cascade));
}

uint select_cascade(float3 world_pos, uint cascade_count, float base_spacing, float spacing_scale) {
    for(uint c = 0; c < cascade_count; ++c) {
        if(in_range(world_pos, cascade_probe_spacing(c, base_spacing, spacing_scale))) {
            return c;
        }
    }
    return cascade_count - 1;
}

uint global_probe_index(uint cascade, uint local_probe_index) {
    return cascade * probes_per_cascade + local_probe_index;
}

void decode_global_probe_index(uint global_probe_index, out uint cascade, out uint local_probe_index) {
    cascade = global_probe_index / probes_per_cascade;
    local_probe_index = global_probe_index % probes_per_cascade;
}


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

uint2 radiance_atlas_coord(uint global_probe_index) {
    return uint2(global_probe_index % probes_per_atlas_row, global_probe_index / probes_per_atlas_row) * radiance_probe_size;
}

uint2 irradiance_atlas_coord(uint global_probe_index) {
    return uint2(global_probe_index % probes_per_atlas_row, global_probe_index / probes_per_atlas_row) * irradiance_probe_size;
}

bool in_range(float3 world_pos, float probe_spacing) {
    const float3 extent = (grid_size - 1) * 0.5 * probe_spacing;
    return all(abs(world_pos) <= extent + probe_spacing);
}

bool should_update_probe(uint probe_index, uint frame_id, uint probe_update_stride) {
    return (probe_index % probe_update_stride) == (frame_id % probe_update_stride);
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
        variance_bias = 0.0001;
        chebyshev_bias = 0.0;
    }
};


float3 encode_irradiance(float3 irr) {
    return sqrt(max(irr, 0.0));
}

float3 decode_irradiance(float3 irr) {
    return sqr(irr);
}

float3 sample_irradiance(
    float3 world_pos,
    float3 normal,
    float3 cam_pos,
    float base_probe_spacing,
    float spacing_scale,
    uint cascade_count,
    Sampler2D<float3> irradiance_tex,
    Sampler2D<float2> distance_tex,
    IrradianceFieldSampleParams params = IrradianceFieldSampleParams()) {

    const uint cascade = ddgi.select_cascade(world_pos, cascade_count, base_probe_spacing, spacing_scale);
    const float probe_spacing = ddgi.cascade_probe_spacing(cascade, base_probe_spacing, spacing_scale);

    return sample_irradiance_cascade(
        world_pos,
        normal,
        cam_pos,
        probe_spacing,
        cascade,
        cascade_count,
        irradiance_tex,
        distance_tex,
        params
    );
}

float3 sample_irradiance_cascade(
    float3 world_pos,
    float3 normal,
    float3 cam_pos,
    float probe_spacing,
    uint cascade,
    uint cascade_count,
    Sampler2D<float3> irradiance_tex,
    Sampler2D<float2> distance_tex,
    IrradianceFieldSampleParams params = IrradianceFieldSampleParams()) {

    if(!ddgi.in_range(world_pos, probe_spacing)) {
        return float3(0.0);
    }

    const float2 inv_radiance_atlas_size = 1.0 / float2(ddgi.radiance_atlas_size(cascade_count));
    const float2 inv_irradiance_atlas_size = 1.0 / float2(ddgi.irradiance_atlas_size(cascade_count));
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
        const uint local_probe_index = ddgi.compute_probe_index(probe_id);
        const uint probe_index = ddgi.global_probe_index(cascade, local_probe_index);

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

        const uint2 irradiance_corner = ddgi.irradiance_atlas_coord(probe_index);
        const float2 sample_uv = (float2(irradiance_corner) + irr_sample_coord + 0.5) * inv_irradiance_atlas_size;
        const float3 irradiance = irradiance_tex.SampleLevel(sample_uv, 0.0);

        total += weight;
        acc += encode_irradiance(irradiance) * weight;
    }

    return total > constants.epsilon ? decode_irradiance(acc / total) : float3(0.0);
}


#endif // DDGI_SLANG

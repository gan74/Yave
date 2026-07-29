#ifndef DDGI_SLANG
#define DDGI_SLANG

#include "utils.slang"


namespace ddgi {

static const uint grid_size = 32;
static const uint radiance_probe_size = 32;
static const uint irradiance_probe_size = radiance_probe_size / 2;
static const uint probe_border = 1;
static const uint radiance_probe_data_size = radiance_probe_size - 2 * probe_border;
static const uint irradiance_probe_data_size = irradiance_probe_size - 2 * probe_border;
static const uint probes_per_atlas_row = 256;

static const uint grid_cell_count = grid_size * grid_size * grid_size;
static const uint invalid_probe_index = 0xFFFFFFFF;


uint2 probe_grid_size() {
    return uint2(grid_size * grid_size, grid_size);
}

uint grid_local_index(int3 grid_coord) {
    return uint(grid_coord.x) * (grid_size * grid_size) + uint(grid_coord.y) * grid_size + uint(grid_coord.z);
}

uint2 probe_grid_texel(int3 grid_coord) {
    return probe_grid_texel(grid_local_index(grid_coord));
}

uint2 probe_grid_texel(uint local_probe_index) {
    return uint2(local_probe_index % (grid_size * grid_size), local_probe_index / (grid_size * grid_size));
}

uint load_probe_index(Sampler2D<uint> probe_grid, int3 grid_coord) {
    return probe_grid[probe_grid_texel(grid_coord)];
}

uint2 radiance_atlas_size(uint atlas_probe_count) {
    return uint2(
        probes_per_atlas_row * radiance_probe_size,
        ((atlas_probe_count + probes_per_atlas_row - 1) / probes_per_atlas_row) * radiance_probe_size
    );
}

uint2 irradiance_atlas_size(uint atlas_probe_count) {
    return uint2(
        probes_per_atlas_row * irradiance_probe_size,
        ((atlas_probe_count + probes_per_atlas_row - 1) / probes_per_atlas_row) * irradiance_probe_size
    );
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

uint2 radiance_atlas_coord(uint atlas_probe_index) {
    return uint2(atlas_probe_index % probes_per_atlas_row, atlas_probe_index / probes_per_atlas_row) * radiance_probe_size;
}

uint2 irradiance_atlas_coord(uint atlas_probe_index) {
    return uint2(atlas_probe_index % probes_per_atlas_row, atlas_probe_index / probes_per_atlas_row) * irradiance_probe_size;
}

bool is_probe_border(uint2 probe_local, uint probe_texel_size) {
    return probe_local.x < probe_border
        || probe_local.y < probe_border
        || probe_local.x >= probe_texel_size - probe_border
        || probe_local.y >= probe_texel_size - probe_border;
}

uint probe_border_texel_count(uint probe_texel_size) {
    return 4u * probe_texel_size - 4u;
}

uint2 probe_border_local(uint border_index, uint probe_texel_size) {
    const uint edge_length = probe_texel_size;
    const uint interior_length = probe_texel_size - 2u * probe_border;

    if(border_index < edge_length) {
        return uint2(border_index, 0u);
    }
    border_index -= edge_length;
    if(border_index < edge_length) {
        return uint2(border_index, probe_texel_size - 1u);
    }
    border_index -= edge_length;
    if(border_index < interior_length) {
        return uint2(0u, border_index + probe_border);
    }
    border_index -= interior_length;
    return uint2(probe_texel_size - 1u, border_index + probe_border);
}

uint2 probe_border_source_local(uint2 probe_local, uint probe_texel_size) {
    const uint interior_min = probe_border;
    const uint interior_max = probe_texel_size - probe_border - 1;
    const uint last = probe_texel_size - 1;

    const bool on_left = probe_local.x < probe_border;
    const bool on_right = probe_local.x >= probe_texel_size - probe_border;
    const bool on_top = probe_local.y < probe_border;
    const bool on_bottom = probe_local.y >= probe_texel_size - probe_border;

    if(on_top && on_left) {
        return uint2(interior_max, interior_max);
    }
    if(on_top && on_right) {
        return uint2(interior_min, interior_max);
    }
    if(on_bottom && on_left) {
        return uint2(interior_max, interior_min);
    }
    if(on_bottom && on_right) {
        return uint2(interior_min, interior_min);
    }

    if(on_top || on_bottom) {
        return uint2(last - probe_local.x, on_top ? interior_min : interior_max);
    }

    return uint2(on_left ? interior_min : interior_max, last - probe_local.y);
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
        sampling_normal_bias = 0.02;
        sampling_view_dir_bias = 0.02;
        distance_bias = 0.01;
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



struct DDGISamplePos {
    float3 biased_world_pos;
    int3 lo;
    int3 hi;

    static DDGISamplePos make_pos(float3 world_pos, float3 normal, float3 view_dir, float probe_spacing, IrradianceFieldSampleParams params = IrradianceFieldSampleParams()) {
        const float3 bias = normal * params.sampling_normal_bias + view_dir * params.sampling_view_dir_bias;

        DDGISamplePos pos;
        pos.biased_world_pos = world_pos + bias * probe_spacing;
        pos.lo = ddgi.compute_grid_coord_low(pos.biased_world_pos, probe_spacing);
        pos.hi = ddgi.compute_grid_coord_high(pos.biased_world_pos, probe_spacing);
        return pos;
    }

    int3 corner(uint k) {
        return int3(
            ((k & 0x01) != 0) ? hi.x : lo.x,
            ((k & 0x02) != 0) ? hi.y : lo.y,
            ((k & 0x04) != 0) ? hi.z : lo.z
        );
    }
};


float3 sample_irradiance(
    float3 world_pos,
    float3 normal,
    float3 cam_pos,
    float probe_spacing,
    uint atlas_probe_count,
    Sampler2D<uint> probe_grid,
    Sampler2D<float3> irradiance_tex,
    Sampler2D<float2> distance_tex,
    IrradianceFieldSampleParams params = IrradianceFieldSampleParams()) {

    if(!ddgi.in_range(world_pos, probe_spacing)) {
        return float3(0.0);
    }

    const float2 inv_radiance_atlas_size = 1.0 / float2(ddgi.radiance_atlas_size(atlas_probe_count));
    const float2 inv_irradiance_atlas_size = 1.0 / float2(ddgi.irradiance_atlas_size(atlas_probe_count));
    const float2 irr_sample_coord = octahedron_encode(normal) * float(ddgi.irradiance_probe_data_size);

    const DDGISamplePos sample_pos = DDGISamplePos.make_pos(world_pos, normal, normalize(cam_pos - world_pos), probe_spacing, params);

    float total = 0.0;
    float3 acc = 0.0;
    for(uint k = 0; k != 8; ++k) {
        const int3 probe_id = sample_pos.corner(k);

        const float3 probe_pos = ddgi.probe_world_position(probe_id, probe_spacing);
        const float3 probe_vec = probe_pos - sample_pos.biased_world_pos;

        const uint probe_index = ddgi.load_probe_index(probe_grid, probe_id);
        if(probe_index == ddgi.invalid_probe_index) {
            continue;
        }

        float weight = 1.0;

        {
            const uint2 dist_corner = ddgi.radiance_atlas_coord(probe_index) + ddgi.probe_border;
            const float2 dist_sample_coord = octahedron_encode(-probe_vec) * float(ddgi.radiance_probe_data_size - 1);
            const float2 dist_sample_uv = (float2(dist_corner) + dist_sample_coord + 0.5) * inv_radiance_atlas_size;

            const float2 dist_moments = distance_tex.SampleLevel(dist_sample_uv, 0.0);
            if(all(dist_moments <= 0.0)) {
                continue;
            }

            const float mean = dist_moments.x + params.distance_bias;
            const float variance = abs(sqr(mean) - dist_moments.y) + params.variance_bias;

            const float norm_dist = length(probe_vec) / (probe_spacing * constants.sqrt_3);
            if(norm_dist > mean) {
                continue;
            }

            /*const float chebyshev = variance / (variance + sqr(max(0.0, norm_dist - mean)));
            const float chebyshev_weight = max(0.0, sqr(chebyshev) - params.chebyshev_bias) / max(1.0 - params.chebyshev_bias, constants.epsilon);
            weight *= (norm_dist <= mean) ? 1.0 : chebyshev_weight;*/
        }

        /*{
            const float3 true_probe_vec = probe_pos - world_pos;
            const float true_dist = length(true_probe_vec);
            if(true_dist > constants.epsilon) {
                weight *= max(0.0, dot(true_probe_vec / true_dist, normal));
            }
        }*/

        /*const float crush_threshold = 0.1;
        if(weight < crush_threshold) {
            weight *= weight * weight * (1.0 / sqr(crush_threshold));
        }*/

        {
            const float3 trilinear = saturate(1.0 - abs(probe_vec) / probe_spacing);
            weight *= trilinear.x * trilinear.y * trilinear.z;
        }

        if(weight > constants.epsilon) {
            const uint2 irradiance_corner = ddgi.irradiance_atlas_coord(probe_index) + ddgi.probe_border;
            const float2 sample_uv = (float2(irradiance_corner) + irr_sample_coord) * inv_irradiance_atlas_size;
            const float3 irradiance = irradiance_tex.SampleLevel(sample_uv, 0.0);

            total += weight;
            acc += encode_irradiance(irradiance) * weight;
        }
    }

    return total > constants.epsilon ? decode_irradiance(acc / total) : float3(0.0);
}


#endif // DDGI_SLANG

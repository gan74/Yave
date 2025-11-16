#ifndef GI_SLANG
#define GI_SLANG

#include "utils.slang"
#include "rt.slang"


namespace ddgi {

static const uint probe_rays = 128;

static const uint grid_size = 32;
static const uint atlas_size = 256;

static const uint probe_size_no_border = 14;
static const uint probe_size_border = probe_size_no_border + 2;

uint compute_probe_index(int3 grid_coord) {
    return (grid_coord.x * grid_size * grid_size) + (grid_coord.y * grid_size) + grid_coord.z;
}

int3 compute_grid_coord(float3 world_pos) {
    return int3(round(world_pos)) % grid_size;
}

int3 compute_grid_coord_low(float3 world_pos) {
    return int3(floor(world_pos)) % grid_size;
}

int3 compute_grid_coord_high(float3 world_pos) {
    return int3(ceil(world_pos)) % grid_size;
}

int3 compute_grid_coord(uint probe_index) {
    const int3 coord = int3(
        (probe_index / (grid_size * grid_size)) % grid_size,
        (probe_index / grid_size) % grid_size,
        probe_index % grid_size
    );
    return coord;
}

int3 compute_world_coord(int3 grid_coord, float3 cam_pos) {
    const int3 c = int3(round(cam_pos)) + (grid_size / 2);
    const int3 n = (c - grid_coord) / grid_size;
    return grid_coord + n * grid_size;
}

uint2 atlas_coord(uint probe_index) {
    return uint2(probe_index / atlas_size, probe_index % atlas_size) * probe_size_border;
}

bool in_range(float3 world_pos, float3 cam_pos) {
    const int3 delta = int3(round(world_pos)) - int3(round(cam_pos));
    return all(abs(delta) < grid_size / 2);
}

}




float3 encode_irr(float3 irr) {
    return sqrt(irr);
}

float3 decode_irr(float3 irr) {
    return sqr(irr);
}


// #define DDGI_SAMPLE_EXTENDED


float3 sample_irradiance(
    float3 world_pos,
    float3 normal,
    float3 cam_pos,
    Sampler2D<float3> irradiance_tex,
    Sampler2D<float2> distance_tex,
    IrradianceFieldSampleParams params = IrradianceFieldSampleParams()) {

    const float2 inv_atlas_size = 1.0 / image_size(irradiance_tex);

    const float2 sample_coord = octahedron_encode(normal) * ddgi.probe_size_no_border;

    const float3 view_dir = normalize(cam_pos - world_pos);
    const float3 bias = normal * params.sampling_normal_bias + view_dir * params.sampling_view_dir_bias;
    const float3 biased_world_pos = world_pos + bias;

    float total = 0.0;
    float3 acc = 0.0;

#ifdef DDGI_SAMPLE_EXTENDED
    for(int kx = -1; kx <= 1; ++kx)
    for(int ky = -1; ky <= 1; ++ky)
    for(int kz = -1; kz <= 1; ++kz) {
        const int3 probe_id = ddgi.compute_grid_coord(biased_world_pos + float3(kx, ky, kz));
#else
    const int3 lo = ddgi.compute_grid_coord_low(biased_world_pos);
    const int3 hi = ddgi.compute_grid_coord_high(biased_world_pos);
    for(uint k = 0; k != 8; ++k) {
        const int3 probe_id = int3(
            ((k & 0x01) != 0) ? lo.x : hi.x,
            ((k & 0x02) != 0) ? lo.y : hi.y,
            ((k & 0x04) != 0) ? lo.z : hi.z,
        );
#endif


        const int3 probe_pos = ddgi.compute_world_coord(probe_id, cam_pos);
        const float3 probe_vec = probe_pos - biased_world_pos;
        const float probe_dist = length(probe_vec);
        const uint probe_index = ddgi.compute_probe_index(probe_id);
        const uint2 probe_corner = ddgi.atlas_coord(probe_index) + 1;

        float weight = 1.0;

        {
            const float2 dist_sample_coord = octahedron_encode(probe_vec) * ddgi.probe_size_no_border;
            const float2 dist_sample_uv = (probe_corner + dist_sample_coord) * inv_atlas_size;

            const float2 dist_moments = distance_tex.SampleLevel(dist_sample_uv, 0.0);
            if(all(dist_moments == 0.0)) {
                continue;
            }

            const float mean = dist_moments.x + params.distance_bias;
            const float variance = abs(sqr(mean) - dist_moments.y) + params.variance_bias;

            const float chebyshev = variance / (variance + sqr(max(0.0, probe_dist - mean)));
            const float chebyshev_weight = max(0.0, sqr(chebyshev) - params.chebyshev_bias) / (1.0 - params.chebyshev_bias);
            weight *= (probe_dist <= mean) ? 1.0 : chebyshev_weight;
        }

#if 1
        {
            const float3 true_probe_vec = probe_pos - world_pos;
            weight *= max(0.0, dot(normalize(true_probe_vec), normal));
            // weight *= max(0.0, dot(normalize(true_probe_vec), normal) * 0.5 + 0.5);
        }
#endif

        const float crush_threshold = 0.2;
        if (weight < crush_threshold) {
            weight = max(constants.epsilon, weight);
            weight *= weight * weight * (1.0 / sqr(crush_threshold));
        }

        {
#ifdef DDGI_SAMPLE_EXTENDED
            const float3 trilinear = saturate(1.0 - abs(probe_vec) / 1.5);
#else
            const float3 trilinear = saturate(1.0 - abs(probe_vec));
#endif
            weight *= trilinear.x * trilinear.y * trilinear.z;
        }


        const float2 sample_uv = (probe_corner + sample_coord) * inv_atlas_size;
        const float3 irr = irradiance_tex.SampleLevel(sample_uv, 0.0);

#if 0
        if(weight > total) {
            total = weight;
            acc = encode_irr(identifying_color(probe_index)) * weight;
        }
#else
        total += weight;
        acc += encode_irr(irr) * weight;
#endif
    }


    return total > constants.epsilon ? (decode_irr(acc / total) * 0.5 * constants.pi) : float3(0.0);
}





#endif // GI_SLANG

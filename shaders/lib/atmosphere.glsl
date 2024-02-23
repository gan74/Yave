#include "utils.glsl"

const float lut_first_slice = 16.0 / 1000.0;
const float lut_slice_factor = 11.0f;

float lut_dist(AtmosphereParams params, float w) {
    return lut_first_slice * (exp(w * lut_slice_factor) - 1.0);
}

float lut_w(AtmosphereParams params, float dist) {
    return log(dist / lut_first_slice + 1.0) / lut_slice_factor;
}

float atmosphere_density(AtmosphereParams params, vec3 pos) {
    const float height_above_ground = length(pos - params.center) - params.planet_radius;
    const float normalized_atmo_height = saturate(height_above_ground / params.atmosphere_height);
    return exp(-normalized_atmo_height * params.density_falloff) * (1.0 - normalized_atmo_height);
}

float optical_depth(AtmosphereParams params, vec3 orig, vec3 dir, float len, uint sample_count) {
    const float step_size = len / (sample_count - 1);
    const vec3 step = dir * step_size;

    float depth = 0.0;
    vec3 pos = orig;
    for(uint i = 0; i != sample_count; ++i) {
        const float density = atmosphere_density(params, pos);
        depth += density * step_size;
        pos += step;
    }

    return depth;
}

void compute_atmospheric_scattering_iter(AtmosphereParams params, vec3 orig, vec3 dir, float len, inout vec3 in_scattered, inout float ray_optical_depth) {
    const uint step_count = 16;
    const uint optical_depth_step_count = 16;

    const float step_size = len / (step_count - 1);
    const vec3 step = dir * step_size;

    vec3 pos = orig;
    for(uint i = 0; i != step_count; ++i) {
        ray_optical_depth += optical_depth(params, pos, -dir, step_size * i, optical_depth_step_count);

        // replace by intersect_ray_sphere
        if(intersect_sphere(params.center, params.planet_radius, pos, params.sun_dir).x < 0.0) {
            const float sun_dist = intersect_sphere(params.center, params.radius, pos, params.sun_dir).y;
            const float sun_optical_depth = optical_depth(params, pos, params.sun_dir, sun_dist, optical_depth_step_count);
            const vec3 transmittance = exp(params.scattering_coeffs * -(sun_optical_depth + ray_optical_depth));
            in_scattered += atmosphere_density(params, pos) * transmittance * params.scattering_coeffs * step_size;
        }
        pos += step;
    }

}



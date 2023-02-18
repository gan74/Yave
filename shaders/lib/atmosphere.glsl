#include "utils.glsl"

struct AtmosphereParams {
    vec3 center;
    float planet_radius;

    vec3 scattering_coeffs;
    float atmosphere_height;

    vec3 sun_dir;
    float radius; // planet_radius + atmosphere_height

    vec3 sun_color;
    float density_falloff;
};


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


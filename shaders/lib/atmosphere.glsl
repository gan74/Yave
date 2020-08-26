#ifndef ATMOSPHERE_GLSL
#define ATMOSPHERE_GLSL

#include "utils.glsl"
/*
float atmosphere_density(float normalized_altitude) {
    return exp(-normalized_altitude * density_falloff) * (1.0 - normalized_altitude);
}

float atmosphere_density(Atmosphere atmo, vec3 pos) {
    const float altitude = length(center - pos) - planet_radius;
    const float normalized_alt = altitude / (radius - planet_radius);
    return atmosphere_density(normalized_alt);
}

float optical_depth(Atmosphere atmo, vec3 orig, vec3 dir, float len) {
    const float step_size = len / (sample_count - 1);
    const vec3 step = dir * step_size;

    float depth = 0.0;
    vec3 pos = orig;
    for(uint i = 0; i != sample_count; ++i) {
        const float density = atmosphere_density(pos);
        depth += density * step_size;
        pos += step;
    }

    return depth;
}*/

#endif


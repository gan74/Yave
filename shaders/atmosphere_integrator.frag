#version 450

#include "lib/atmosphere.glsl"

layout(set = 0, binding = 0) uniform AtmosphereData_Inline {
    AtmosphereParams atmos;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out float out_optical_depth;



const uint sample_count = 64;

void main() {
    const float cos_t = in_uv.x * 2.0 - 1.0;
    const float sin_t = sqrt(1.0 - sqr(cos_t));
    const vec3 dir = vec3(sin_t, 0.0, cos_t);

    const float normalized_altitude = in_uv.y;
    const float altitude = atmos.planet_radius + normalized_altitude * atmos.atmosphere_height;
    const vec3 orig = atmos.center + vec3(0.0, 0.0, altitude);

    out_optical_depth = optical_depth(atmos, orig, dir, 2.0 * atmos.radius, sample_count);
}


#version 450

#include "lib/atmosphere.glsl"

// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform Falloff {
    float density_falloff;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out float out_color;


const uint sample_count = 32;


// -------------------------------- HELPERS --------------------------------

float atmosphere_density(float normalized_altitude) {
    return exp(-normalized_altitude * density_falloff) * (1.0 - normalized_altitude);
}

float atmosphere_density(vec3 pos) {
    const float normalized_alt = pos.z;
    return atmosphere_density(normalized_alt);
}

float optical_depth(vec3 orig, vec3 dir, float len) {
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
}


// -------------------------------- MAIN --------------------------------

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const float cos_t = in_uv.x * 2.0 - 1.0;
    const float altitude = in_uv.y;

    const float depth = optical_depth(vec3(0.0, 0.0, altitude), vec3(0.0, 0.0, cos_t), 1.0);

    out_color = depth;
}

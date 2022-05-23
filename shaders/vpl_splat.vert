#version 450

#include "lib/vpl.glsl"

layout(location = 0) out vec3 out_color;

layout(set = 0, binding = 0) readonly buffer VPLs {
    VPL vpls[];
};

layout(set = 0, binding = 1) readonly buffer Probes {
    vec4 probe_positions[];
};

layout(set = 0, binding = 2) uniform Params_Inline {
    uvec2 probe_count;
};

vec2 to_probe(vec2 clip) {
    const vec2 probe_offset = vec2(gl_InstanceIndex % probe_count.x, gl_InstanceIndex / probe_count.x);
    const vec2 h = clip * 0.5 + 0.5;
    return ((h + probe_offset) / probe_count) * 2.0 - 1.0;
}

void main() {
    const VPL vpl = vpls[gl_VertexIndex];

    const vec3 center = probe_positions[gl_InstanceIndex].xyz;
    vec3 direction = (vpl.position - center);
    const float distance = length(direction);
    direction /= distance;

    out_color = vpl.color;
    gl_PointSize = 4.0;
    gl_Position = vec4(to_probe(direction.xy), 1.0 / distance, 1.0);

    if(dot(vpl.normal, direction) > 0.0) {
        gl_Position = vec4(uintBitsToFloat(0xFFFFFFFF));
    }
}


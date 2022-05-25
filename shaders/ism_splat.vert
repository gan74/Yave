#version 450

#include "lib/vpl.glsl"

layout(location = 0) out vec3 out_color;

layout(set = 0, binding = 0) readonly buffer VBO {
    float positions[];
};

layout(set = 0, binding = 1) uniform Params_Inline {
    uvec2 probe_count;
};

vec2 to_probe(vec3 dir) {
    const vec2 probe_offset = vec2(gl_InstanceIndex % probe_count.x, gl_InstanceIndex / probe_count.x);
    const vec2 h = octahedron_encode(dir);
    return ((h + probe_offset) / probe_count) * 2.0 - 1.0;
}

void main() {
    const uint point_index = gl_VertexIndex.x * 3 * 4;

    const vec3 center = vec3(0.0);
    const vec3 pos = vec3(
        positions[point_index + 0],
        positions[point_index + 1],
        positions[point_index + 2]
    );

    vec3 direction = pos - center;
    const float distance = length(direction);
    direction /= distance;


    out_color = vec3(1.0);
    gl_PointSize = 1.0;
    gl_Position = vec4(to_probe(direction), 1.0 / distance, 1.0);

}


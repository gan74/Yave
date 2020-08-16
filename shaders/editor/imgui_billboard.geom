#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(set = 0, binding = 0) uniform sampler2D font_texture;
layout(set = 0, binding = 1) uniform Buffer {
    mat4 view_proj;
    vec2 viewport_size;
    float billboard_size;
};

vec2 uvs[] = vec2[](
        vec2(0.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0)
    );

layout(location = 0) in flat uint in_instance_id[];
layout(location = 1) in vec3 in_position[];
layout(location = 2) in vec2 in_uv[];
layout(location = 3) in vec2 in_size[];

layout(location = 0) out flat uint out_instance_id;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec4 out_color;

void main() {
    const vec4 position = view_proj * vec4(in_position[0], 1.0);
    const vec2 size = (billboard_size * position.w) / viewport_size;

    for(uint i = 0; i != 4; ++i) {
        out_instance_id = in_instance_id[0];
        out_uv = in_uv[0] + uvs[i] * in_size[0];
        out_color = vec4(1.0);
        gl_Position = vec4(position.xy + (uvs[i] - 0.5) * size, position.zw);
        EmitVertex();
    }
}


#version 450

layout(set = 0, binding = 0) uniform ViewProj {
    mat4 view_proj;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 8) in mat4 in_model;
layout(location = 12) in uint in_id;

layout(location = 0) out uint out_instance_id;
layout(location = 1) out vec2 out_uv;
layout(location = 2) out vec4 out_color;

void main() {
    const mat3 model = mat3(in_model);
    gl_Position = view_proj * in_model * vec4(in_position, 1.0);

    out_instance_id = in_id;
    out_uv = vec2(0.0);
    out_color = vec4(1.0);
}


#version 450

#include "lib/utils.glsl"

layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 8) in mat4 in_model;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_tangent;
layout(location = 2) out vec3 out_bitangent;
layout(location = 3) out vec2 out_uv;

void main() {
    out_uv = in_uv;

    const mat3 model = mat3(in_model);
    out_normal = model * in_normal;
    out_tangent = model * in_tangent;
    out_bitangent = cross(out_tangent, out_normal);

    gl_Position = camera.view_proj * in_model * vec4(in_position, 1.0);
}


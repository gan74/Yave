#version 450

#include "lib/utils.glsl"

layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(set = 1, binding = 0) uniform Bones {
    mat4 bone_transforms[max_bones];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 4) in uvec4 in_skin_indexes;
layout(location = 5) in vec4 in_skin_weights;

layout(location = 8) in mat4 in_model;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_tangent;
layout(location = 2) out vec3 out_bitangent;
layout(location = 3) out vec2 out_uv;

void main() {
    const mat4 bone_matrix = in_skin_weights.x * bone_transforms[in_skin_indexes.x] +
                             in_skin_weights.y * bone_transforms[in_skin_indexes.y] +
                             in_skin_weights.z * bone_transforms[in_skin_indexes.z] +
                             in_skin_weights.w * bone_transforms[in_skin_indexes.w];

    out_uv = in_uv;
    out_normal = mat3(in_model) * mat3(bone_matrix) * in_normal;
    out_normal = mat3(in_model) * mat3(bone_matrix) * in_tangent;
    out_bitangent = cross(out_tangent, out_normal);
    gl_Position = camera.view_proj * in_model * bone_matrix * vec4(in_position, 1.0);
}


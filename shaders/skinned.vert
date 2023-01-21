#version 450

#include "lib/utils.glsl"

layout(set = 0, binding = 0) uniform CameraData {
    Camera camera;
};

layout(set = 1, binding = 0) uniform Bones {
    mat4 bone_transforms[max_bones];
};

layout(location = 0) in vec3 in_position;
layout(location = 1) in uvec2 in_packed_normal_tangent_sign;
layout(location = 2) in vec2 in_uv;

layout(location = 3) in uvec4 in_skin_indices;
layout(location = 4) in vec4 in_skin_weights_Packed;

layout(location = 8) in mat4 in_model;

layout(location = 0) out vec3 out_normal;
layout(location = 1) out vec3 out_tangent;
layout(location = 2) out vec3 out_bitangent;
layout(location = 3) out vec2 out_uv;

void main() {
    const mat4 bone_matrix = in_skin_weights_Packed.x * bone_transforms[in_skin_indices.x] +
                             in_skin_weights_Packed.y * bone_transforms[in_skin_indices.y] +
                             in_skin_weights_Packed.z * bone_transforms[in_skin_indices.z] +
                             in_skin_weights_Packed.w * bone_transforms[in_skin_indices.w];

    out_uv = in_uv;

    const vec3 in_normal = unpack_2_10_10_10(in_packed_normal_tangent_sign.x).xyz;
    const vec4 in_tangent_sign = unpack_2_10_10_10(in_packed_normal_tangent_sign.y);

    out_normal = mat3(in_model) * mat3(bone_matrix) * in_normal;
    out_tangent = mat3(in_model) * mat3(bone_matrix) * in_tangent_sign.xyz;
    out_bitangent = cross(out_tangent, out_normal) * in_tangent_sign.w;

    gl_Position = camera.view_proj * in_model * bone_matrix * vec4(in_position, 1.0);
}


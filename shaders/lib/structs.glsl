#ifndef STRUCTS_GLSL
#define STRUCTS_GLSL

struct Camera {
    mat4 view_proj;
    mat4 inv_view_proj;

    mat4 unjittered_view_proj;
    mat4 inv_unjittered_view_proj;
    mat4 prev_unjittered_view_proj;

    mat4 proj;
    mat4 inv_proj;

    mat4 view;
    mat4 inv_view;

    vec3 position;
    uint padding_0;

    vec3 forward;
    uint padding_1;

    vec3 up;
    uint padding_2;
};

struct TransformableData {
    mat4 current;
    mat4 last;
};

const uint diffuse_texture_index = 0;
const uint normal_texture_index = 1;
const uint emissive_texture_index = 2;
const uint metallic_roughness_texture_index = 3;
const uint specular_texture_index = 3;
const uint specular_color_texture_index = 4;

struct MaterialData {
    vec3 emissive_factor;
    float roughness_factor;

    vec3 base_color_factor;
    float metallic_factor;

    vec3 specular_color_factor;
    float specular_factor;

    uint texture_indices[8];
};

struct DirectionalLight {
    vec3 direction;
    float cos_disk;

    vec3 color;
    uint padding_1;

    uvec4 shadow_map_indices;
};

struct PointLight {
    vec3 position;
    float range;

    vec3 color;
    float falloff;

    vec3 padding_0;
    float min_radius;
};

struct SpotLight {
    vec3 position;
    float range;

    vec3 color;
    float falloff;

    vec3 forward;
    float min_radius;

    vec2 att_scale_offset;
    float sin_angle;
    uint shadow_map_index;

    vec3 encl_sphere_center;
    float encl_sphere_radius;
};

struct ShadowMapInfo {
    mat4 view_proj;

    vec2 uv_offset;
    vec2 uv_mul;

    float size;
    float texel_size;
    uint padding_0;
    uint padding_1;
};

struct ExposureParams {
    float exposure;
    float avg_lum;
    float max_lum;

    uint padding_0;
};

struct AtmosphereParams {
    vec3 center;
    float planet_radius;

    vec3 scattering_coeffs;
    float atmosphere_height;

    vec3 sun_dir;
    float radius; // planet_radius + atmosphere_height

    vec3 sun_color;
    float density_falloff;
};

#endif

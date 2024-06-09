#version 460

#include "lib/utils.glsl"
#include "lib/gbuffer.glsl"
#include "lib/interpolants.glsl"

#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec2 out_motion;
layout(location = 1) out vec4 out_rt0;
layout(location = 2) out vec4 out_rt1;
layout(location = 3) out vec3 out_emissive;

layout(set = 1, binding = 1) readonly buffer Materials {
    MaterialData materials[];
};

layout(set = 1, binding = 2) readonly buffer Indices {
    uvec2 mesh_indices[];
};


layout(set = 2, binding = 0) uniform sampler2D all_textures_Variable[];


DECLARE_STANDARD_INTERPOLANTS(in)




// https://github.com/nvpro-samples/vk_raytrace/blob/master/shaders/gltf_material.glsl
float perceived_brightness(vec3 color) {
    return sqrt(dot(vec3(0.299, 0.587, 0.114), color * color));
}

float metalic_from_specular(vec3 diffuse, vec3 specular) {
    const float min_reflectance = 0.04;

    const float reflectance = max(max(specular.r, specular.g), specular.b);
    const float one_minus_spec_strength = saturate(1.0 - reflectance);

    const float specular_brightness = perceived_brightness(specular);
    const float diffuse_brightness = perceived_brightness(diffuse);

    if(specular_brightness < min_reflectance) {
        return 0.0;
    }

    const float a = min_reflectance;
    const float b = diffuse_brightness * one_minus_spec_strength / (1.0 - min_reflectance) + specular_brightness - 2.0 * min_reflectance;
    const float c = min_reflectance - specular_brightness;
    const float D = max(b * b - 4.0 * min_reflectance * c, 0.0);

    return saturate((-b + sqrt(D)) / (2.0 * a));
}





vec4 tex_from_index(MaterialData material, uint index, vec2 uv) {
    const uint texture_index = material.texture_indices[index];
    return texture(all_textures_Variable[nonuniformEXT(texture_index)], uv);
}

void main() {
    const MaterialData material = materials[mesh_indices[in_instance_index].y];

    const vec4 color = tex_from_index(material, diffuse_texture_index, in_uv);

#ifdef ALPHA_TEST
    if(color.a < 0.5) {
        discard;
    }
#endif


    const vec3 normal = unpack_normal_map(tex_from_index(material, normal_texture_index, in_uv).xy);
    const vec3 mapped_front_face_normal = normal.x * in_tangent +
                                          normal.y * in_bitangent +
                                          normal.z * in_normal;

    const vec3 mapped_normal = gl_FrontFacing ? mapped_front_face_normal : -mapped_front_face_normal;

    SurfaceInfo surface;
    surface.albedo = color.rgb * material.base_color_factor;
    surface.normal = mapped_normal;

#ifdef SPECULAR
    const float gloss = tex_from_index(material, specular_texture_index, in_uv).a * material.specular_factor;
    const vec3 specular = tex_from_index(material, specular_color_texture_index, in_uv).rgb * material.specular_color_factor;
    surface.perceptual_roughness = saturate(1.0 - gloss);
    surface.metallic = metalic_from_specular(surface.albedo, specular);
#else
    const vec4 metallic_roughness = tex_from_index(material, metallic_roughness_texture_index, in_uv);
    surface.perceptual_roughness = metallic_roughness.g * material.roughness_factor;
    surface.metallic = metallic_roughness.b * material.metallic_factor;
#endif


    write_gbuffer(surface, out_rt0, out_rt1);

    out_motion = ((in_last_screen_pos.xy / in_last_screen_pos.z) - (in_screen_pos.xy / in_screen_pos.z)) * 0.5;
    out_emissive = tex_from_index(material, emissive_texture_index, in_uv).rgb * material.emissive_factor;
}


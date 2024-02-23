#version 450

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


#define GLTF_ROUGHNESS_CHANNEL g
#define GLTF_METALLIC_CHANNEL b

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
    const vec3 mapped_normal = normal.x * in_tangent +
                               normal.y * in_bitangent +
                               normal.z * in_normal;

    SurfaceInfo surface;
    surface.albedo = color.rgb * material.base_color_mul;
    surface.normal = mapped_normal;

    // We fetch Y so we can use either greyscale or RG metallic/roughness textures
    surface.perceptual_roughness = tex_from_index(material, roughness_texture_index, in_uv).GLTF_ROUGHNESS_CHANNEL * material.roughness_mul;
    surface.metallic = tex_from_index(material, metallic_texture_index, in_uv).GLTF_METALLIC_CHANNEL * material.metallic_mul;


    write_gbuffer(surface, out_rt0, out_rt1);

    out_motion = ((in_last_screen_pos.xy / in_last_screen_pos.z) - (in_screen_pos.xy / in_screen_pos.z)) * 0.5;
    out_emissive = tex_from_index(material, emissive_texture_index, in_uv).rgb * material.emissive_mul;
}


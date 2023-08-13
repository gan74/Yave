#include "lib/utils.glsl"
#include "lib/gbuffer.glsl"

#extension GL_EXT_nonuniform_qualifier : enable

#define EMISSIVE

layout(location = 0) out vec4 out_rt0;
layout(location = 1) out vec4 out_rt1;

#ifdef EMISSIVE
layout(location = 2) out vec4 out_emissive;
#endif

layout(set = 1, binding = 1) readonly buffer Materials {
    MaterialData materials[];
};

layout(set = 1, binding = 2) readonly buffer Indices {
    uvec2 mesh_indices[];
};


layout(set = 2, binding = 0) uniform sampler2D all_textures_Variable[];

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_tangent;
layout(location = 2) in vec3 in_bitangent;
layout(location = 3) in vec2 in_uv;
layout(location = 4) in flat uint in_instance_index;




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
    surface.albedo = color.rgb;
    surface.normal = mapped_normal;

    // We fetch Y so we can use either greyscale or RG metallic/roughness textures
    surface.perceptual_roughness = tex_from_index(material, roughness_texture_index, in_uv).GLTF_ROUGHNESS_CHANNEL * material.roughness_mul;
    surface.metallic = tex_from_index(material, metallic_texture_index, in_uv).GLTF_METALLIC_CHANNEL * material.metallic_mul;

    write_gbuffer(surface, out_rt0, out_rt1);

#ifdef EMISSIVE
    out_emissive = tex_from_index(material, emissive_texture_index, in_uv) * vec4(material.emissive_mul, 1.0);
#endif
}


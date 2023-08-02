#include "lib/utils.glsl"
#include "lib/gbuffer.glsl"

#extension GL_EXT_nonuniform_qualifier : enable

#define EMISSIVE

layout(location = 0) out vec4 out_rt0;
layout(location = 1) out vec4 out_rt1;

#ifdef EMISSIVE
layout(location = 2) out vec4 out_emissive;
#endif

layout(set = 1, binding = 0) uniform sampler2D all_textures_Variable[];

layout(set = 2, binding = 0) uniform MaterialData_Inline {
    vec3 emissive_mul;
    float roughness_mul;
    float metallic_mul;
    uint diffuse_index;
    uint normal_index;
    uint roughness_index;
    uint metallic_index;
    uint emissive_index;
};


layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_tangent;
layout(location = 2) in vec3 in_bitangent;
layout(location = 3) in vec2 in_uv;


#define GLTF_ROUGHNESS_CHANNEL g
#define GLTF_METALLIC_CHANNEL b

vec4 tex_from_index(uint index, vec2 uv) {
    if(index == diffuse_index) {
        return vec4(1, 0, 1, 1);
    }
    if(index == normal_index) {
        return vec4(0, 0, 1, 0);
    }
    if(index == roughness_index) {
        return vec4(0.5);
    }
    return vec4(0);
    // sampler2D tex = all_textures[nonuniformEXT(index)];
    // return texture(tex, uv);
}

void main() {
    const vec4 color = tex_from_index(diffuse_index, in_uv);

#ifdef ALPHA_TEST
    if(color.a < 0.5) {
        discard;
    }
#endif

    const vec3 normal = unpack_normal_map(tex_from_index(normal_index, in_uv).xy);
    const vec3 mapped_normal = normal.x * in_tangent +
                               normal.y * in_bitangent +
                               normal.z * in_normal;

    SurfaceInfo surface;
    surface.albedo = color.rgb;
    surface.normal = mapped_normal;

    // We fetch Y so we can use either greyscale or RG metallic/roughness textures
    surface.perceptual_roughness = tex_from_index(roughness_index, in_uv).GLTF_ROUGHNESS_CHANNEL * roughness_mul;
    surface.metallic = tex_from_index(metallic_index, in_uv).GLTF_METALLIC_CHANNEL * metallic_mul;

    write_gbuffer(surface, out_rt0, out_rt1);

#ifdef EMISSIVE
    out_emissive = tex_from_index(emissive_index, in_uv) * vec4(emissive_mul, 1.0);
#endif
}


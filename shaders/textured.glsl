#include "lib/utils.glsl"
#include "lib/gbuffer.glsl"

layout(location = 0) out vec4 out_rt0;
layout(location = 1) out vec4 out_rt1;

#ifdef EMISSIVE
layout(location = 2) out vec4 out_emissive;
#endif

layout(set = 1, binding = 0) uniform sampler2D in_color;
layout(set = 1, binding = 1) uniform sampler2D in_normal_map;
layout(set = 1, binding = 2) uniform sampler2D in_roughness;
layout(set = 1, binding = 3) uniform sampler2D in_metallic;
layout(set = 1, binding = 4) uniform sampler2D in_emissive;

layout(set = 1, binding = 5) uniform Constants_Inline {
    vec3 emissive_mul;
    float roughness_mul;
    float metallic_mul;
};

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec3 in_tangent;
layout(location = 2) in vec3 in_bitangent;
layout(location = 3) in vec2 in_uv;


#define GLTF_ROUGHNESS_CHANNEL g
#define GLTF_METALLIC_CHANNEL b

void main() {
    const vec4 color = texture(in_color, in_uv);

#ifdef ALPHA_TEST
    if(color.a < 0.5) {
        discard;
    }
#endif

    const vec3 normal = unpack_normal_map(texture(in_normal_map, in_uv).xy);
    const vec3 mapped_normal = normal.x * in_tangent +
                               normal.y * in_bitangent +
                               normal.z * in_normal;

    GBufferData gbuffer;
    gbuffer.albedo = color.rgb;
    gbuffer.normal = mapped_normal;

    // We fetch Y so we can use either greyscale or RG metallic/roughness textures
    gbuffer.roughness = texture(in_roughness, in_uv).GLTF_ROUGHNESS_CHANNEL * roughness_mul;
    gbuffer.metallic = texture(in_metallic, in_uv).GLTF_METALLIC_CHANNEL * metallic_mul;

    write_gbuffer(gbuffer, out_rt0, out_rt1);

#ifdef EMISSIVE
    out_emissive = texture(in_emissive, in_uv) * vec4(emissive_mul, 1.0);
#endif
}


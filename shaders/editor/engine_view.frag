#version 450

#include "../lib/utils.glsl"
#include "../lib/gbuffer.glsl"


layout(set = 0, binding = 0) uniform Target_Inline {
    uint target_index;
};

layout(set = 0, binding = 1) uniform sampler2D in_final;
layout(set = 0, binding = 2) uniform sampler2D in_depth;
layout(set = 0, binding = 3) uniform sampler2D in_rt0;
layout(set = 0, binding = 4) uniform sampler2D in_rt1;
layout(set = 0, binding = 5) uniform sampler2D in_ao;


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const GBufferData gbuffer = read_gbuffer(texelFetch(in_rt0, coord, 0), texelFetch(in_rt1, coord, 0));
    const vec3 final = texelFetch(in_final, coord, 0).rgb;

    vec3 color = final;

    if(target_index == 1) {
        color = gbuffer.albedo;
    } else if(target_index == 2) {
        color = gbuffer.normal * 0.5 + 0.5;
    } else if(target_index == 3) {
        color = vec3(gbuffer.metallic);
    } else if(target_index == 4) {
        color = vec3(gbuffer.roughness);
    } else if(target_index == 5) {
        const float depth = texelFetch(in_depth, coord, 0).r;
        color = vec3(pow(depth, 0.35));
    } else if(target_index == 6) {
        const float ao = texture(in_ao, in_uv).r;
        color = vec3(ao);
    }

    out_color = vec4(color, 1.0);
}


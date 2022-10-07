#version 450

#include "lib/gbuffer.glsl"
#include "lib/screen_space.glsl"

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_rt1;
layout(set = 0, binding = 2) uniform sampler2D in_noise;

layout(set = 0, binding = 3) uniform CameraData {
    Camera camera;
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out float out_ao;

const uint kernel_size = 64;
const float radius = 0.5;
const float bias = 0.025;

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const float depth = texelFetch(in_depth, coord, 0).x;

    const SurfaceInfo surface = read_gbuffer(vec4(0.0), texelFetch(in_rt1, coord, 0));

    const vec3 view_pos = unproject(in_uv, depth, camera.inv_proj);

    const vec3 random_vec = normalize(texelFetch(in_noise, coord % ivec2(256), 0).xyz);
    const vec3 tangent = normalize(random_vec - surface.normal * dot(random_vec, surface.normal));
    const vec3 bitangent = cross(surface.normal, tangent);
    const mat3 TBN = mat3(tangent, bitangent, surface.normal);

    float occlusion = 0.0;
    for(uint i = 0; i != kernel_size; ++i) {
        const vec3 pos = view_pos + (TBN * sample_dirs[i])  * radius;

        const vec3 offset = project(pos, camera.proj);

        const float sample_depth = texture(in_depth, offset.xy).x;
        const vec3 sample_view_pos = unproject(offset.xy, sample_depth, camera.inv_proj);

        const float range_check = smoothstep(0.0, 1.0, radius / abs(pos.z - sample_view_pos.z));
        occlusion += (sample_depth >= depth ? 1.0 : 0.0) * range_check;
    }

    out_ao = 1.0 - (occlusion / kernel_size);
}

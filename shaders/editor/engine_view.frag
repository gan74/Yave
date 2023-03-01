#version 450

#extension GL_EXT_shader_image_int64 : enable

#include "../lib/surfels.glsl"
#include "../lib/gbuffer.glsl"


layout(set = 0, binding = 0) uniform Target_Inline {
    uint target_index;
};

layout(set = 0, binding = 1) uniform sampler2D in_final;
layout(set = 0, binding = 2) uniform sampler2D in_depth;
layout(set = 0, binding = 3) uniform sampler2D in_rt0;
layout(set = 0, binding = 4) uniform sampler2D in_rt1;
layout(set = 0, binding = 5) uniform sampler2D in_ao;

layout(set = 0, binding = 6) uniform CameraData {
    Camera camera;
};

layout(r64ui, set = 0, binding = 7) uniform u64image3D in_ism;

layout(set = 0, binding = 8) readonly buffer Probes {
    vec4 probe_positions[];
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

const uint probe_group_size = 32;

uvec2 compute_probe_count() {
    const uvec2 input_size = uvec2(textureSize(in_depth, 0).xy);
    const uint x = input_size.x / probe_group_size + (input_size.x % probe_group_size == 0 ? 0 : 1);
    const uint y = input_size.y / probe_group_size + (input_size.y % probe_group_size == 0 ? 0 : 1);
    return uvec2(x, y);
}

uint compute_probe_index() {
    const uvec2 group_id = uvec2(gl_FragCoord.xy) / probe_group_size;
    return group_id.y * compute_probe_count().x + group_id.x;
}

uvec2 compute_probe_coord() {
    return uvec2(gl_FragCoord.xy) % probe_group_size;
}

vec2 compute_probe_uv() {
    return vec2(compute_probe_coord()) / vec2(probe_group_size - 1.0);
}

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const float depth = texelFetch(in_depth, coord, 0).r;
    const vec3 world_pos = unproject(in_uv, depth, camera.inv_view_proj);
    const SurfaceInfo surface = read_gbuffer(texelFetch(in_rt0, coord, 0), texelFetch(in_rt1, coord, 0));
    const vec3 final = texelFetch(in_final, coord, 0).rgb;
    const vec3 geom_normal = normalize(cross(dFdy(world_pos), dFdx(world_pos)));

    vec3 color = final;

    if(target_index == 1) {
        color = surface.albedo;
    } else if(target_index == 2) {
        color = surface.normal * 0.5 + 0.5;
    } else if(target_index == 3) {
        color = vec3(surface.metallic);
    } else if(target_index == 4) {
        color = vec3(surface.perceptual_roughness);
    } else if(target_index == 5) {
        color = vec3(pow(depth, 0.35));
    } else if(target_index == 6) {
        const float ao = texture(in_ao, in_uv).r;
        color = vec3(ao);
    } else if(target_index == 7) {
        const uint probe_index = compute_probe_index();
        if(probe_positions[probe_index].w == 0.0) {
            const ivec3 probe_coord = ivec3(compute_probe_coord(), probe_index);
            const float dist = uintBitsToFloat(unpack_64(imageLoad(in_ism, probe_coord).x).x);
            color = vec3(dist / (dist + 10.0));
        }
    } else if(target_index == 8) {
        const vec4 probe = probe_positions[compute_probe_index()];
        if(probe.w == 0.0) {
            const vec3 probe_dir = normalize(probe.xyz - camera.position);
            const vec3 view_dir = -view_direction(camera, in_uv);
            if(dot(view_dir, probe_dir) > 0.99999) {
                const bool hidden = length(world_pos - camera.position) < length(probe.xyz - camera.position);
                color = vec3(hidden ? 0.0 : 1.0);
            }
        }
    }

    out_color = vec4(color, 1.0);
}


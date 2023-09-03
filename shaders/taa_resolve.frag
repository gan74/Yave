#version 450

#include "lib/utils.glsl"


// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_prev;

layout(set = 0, binding = 3) uniform CurrentCameraData {
    Camera current_cam;
};

layout(set = 0, binding = 4) uniform PrevCameraData {
    Camera prev_cam;
};

layout(set = 0, binding = 5) uniform Settings_Inline {
    uint use_reprojection;
    uint use_clamping;
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


// -------------------------------- Utils --------------------------------

struct ClampingInfo {
    vec3 max_color;
    vec3 min_color;
};

void update_clamping(inout ClampingInfo info, vec3 color) {
    info.max_color = max(info.max_color, color);
    info.min_color = min(info.min_color, color);
}

ClampingInfo gather_clamping_info(sampler2D in_color, vec2 uv) {
    ClampingInfo info;
    info.max_color = vec3(0.0);
    info.min_color = vec3(999999.0);

    update_clamping(info, textureOffset(in_color, uv, ivec2(-1, -1)).rgb);
    update_clamping(info, textureOffset(in_color, uv, ivec2(-1,  0)).rgb);
    update_clamping(info, textureOffset(in_color, uv, ivec2(-1,  1)).rgb);

    update_clamping(info, textureOffset(in_color, uv, ivec2( 0, -1)).rgb);
    update_clamping(info, textureOffset(in_color, uv, ivec2( 0,  0)).rgb);
    update_clamping(info, textureOffset(in_color, uv, ivec2( 0,  1)).rgb);

    update_clamping(info, textureOffset(in_color, uv, ivec2( 1, -1)).rgb);
    update_clamping(info, textureOffset(in_color, uv, ivec2( 1,  0)).rgb);
    update_clamping(info, textureOffset(in_color, uv, ivec2( 1,  1)).rgb);

    return info;
}

mat4 unjittered_view_proj(Camera cam) {
    return cam.unjittered_proj * cam.view;
}


// -------------------------------- Main --------------------------------

void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const vec2 uv = gl_FragCoord.xy / vec2(textureSize(in_depth, 0).xy);

    bool sample_valid = true;

    vec2 prev_uv = uv;
    if(use_reprojection != 0) {
        const float depth = texelFetch(in_depth, coord, 0).x;
        const mat4 unproj_mat = inverse(unjittered_view_proj(current_cam));
        const mat4 reproj_mat = unjittered_view_proj(prev_cam);
        const vec3 world_pos = unproject(uv, depth, unproj_mat);
        const vec2 prev_uv = project(world_pos, reproj_mat).xy;

        sample_valid = sample_valid && (prev_uv == saturate(prev_uv));
    }

    const vec3 current = texture(in_color, uv).rgb;
    vec3 prev = texture(in_prev, prev_uv).rgb;

    if(use_clamping != 0 && sample_valid) {
        const ClampingInfo clamping_info = gather_clamping_info(in_color, uv);
        prev = clamp(prev, clamping_info.min_color, clamping_info.max_color);
    }


    out_color = vec4(mix(current, prev, sample_valid ? 0.9 : 0.0), 1.0);
}


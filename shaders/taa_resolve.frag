#version 450

#include "lib/utils.glsl"


// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_prev;
layout(set = 0, binding = 3) uniform sampler2D in_motion;
layout(set = 0, binding = 4) uniform sampler2D in_mask;

layout(set = 0, binding = 5) uniform CameraData {
    Camera camera;
};

layout(set = 0, binding = 6) uniform Settings_Inline {
    uint flags;
    float blending_factor;
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

bool fetch_deocclusion_mask(vec2 uv) {
    const vec4 mask = textureGather(in_mask, uv, 0);
    return any(greaterThan(mask, vec4(0.0)));
}


// -------------------------------- Main --------------------------------

const uint reprojection_bit = 0x1;
const uint clamping_bit = 0x2;
const uint mask_bit = 0x4;

// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);
    const vec2 inv_size = 1.0 / vec2(textureSize(in_color, 0).xy);
    const vec2 uv = gl_FragCoord.xy * inv_size;
    const vec2 motion = texelFetch(in_motion, coord, 0).xy;

    bool sample_valid = true;

    if((flags & mask_bit) != 0) {
        if(motion == vec2(0.0) && fetch_deocclusion_mask(uv)) {
            sample_valid = false;
        }
    }

    vec2 prev_uv = uv + motion;
    if((flags & reprojection_bit) != 0 && sample_valid) {
        const float depth = texelFetch(in_depth, coord, 0).x;
        if(!is_OOB(depth)) {
            // This seems wrong, we should take prev_uv into account before the reproj ?
            const vec3 world_pos = unproject(uv, depth, camera.inv_unjittered_view_proj);
            prev_uv = project(world_pos, camera.prev_unjittered_view_proj).xy + motion;

            sample_valid = sample_valid && (prev_uv == saturate(prev_uv));
        } else {
            sample_valid = false;
        }
    }

    const vec3 current = texelFetch(in_color, coord, 0).rgb;
    vec3 prev = texture(in_prev, prev_uv).rgb;

    if((flags & clamping_bit) != 0 && sample_valid) {
        // Only clamp when UV delta is > 1 pixel
        if(any(greaterThan(abs(prev_uv - uv), inv_size))) {
            const ClampingInfo clamping_info = gather_clamping_info(in_color, uv);
            prev = clamp(prev, clamping_info.min_color, clamping_info.max_color);
        }
    }

    out_color = vec4(mix(current, prev, sample_valid ? blending_factor : 0.0), 1.0);

#if 0
    if(!sample_valid) {
        out_color = vec4(1, 0, 0, 1);
    }
#endif
}


#version 450

#include "lib/utils.glsl"


// -------------------------------- I/O --------------------------------

layout(set = 0, binding = 0) uniform sampler2D in_depth;
layout(set = 0, binding = 1) uniform sampler2D in_color;
layout(set = 0, binding = 2) uniform sampler2D in_prev;
layout(set = 0, binding = 3) uniform sampler2D in_motion;
layout(set = 0, binding = 4) uniform sampler2D in_prev_motion;

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

ClampingInfo gather_clamping_info(sampler2D color, ivec2 coord) {
    ClampingInfo info;
    info.max_color = vec3(0.0);
    info.min_color = vec3(999999.0);

    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const vec3 smp = texelFetch(color, coord + ivec2(x, y), 0).rgb;
            info.max_color = max(info.max_color, smp);
            info.min_color = min(info.min_color, smp);
        }
    }

    return info;
}

vec2 find_motion(sampler2D depth, sampler2D motion, ivec2 coord) {
#if 1
    return texelFetch(motion, coord, 0).xy;
#else
    ivec2 best_coord = coord;
    float closest = 0.0;

    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const float smp = texelFetch(depth, coord + ivec2(x, y), 0).x;
            if(smp > closest) {
                closest = smp;
                best_coord = coord + ivec2(x, y);
            }
        }
    }

    return texelFetch(motion, best_coord, 0).xy;
#endif
}


// -------------------------------- Main --------------------------------

const uint clamping_bit = 0x1;
const uint motion_rejection_bit = 0x2;

// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const vec2 size = vec2(textureSize(in_color, 0).xy);
    const vec2 inv_size = 1.0 / size;
    const float y_ratio = size.y * inv_size.x;

    const vec2 motion = find_motion(in_depth, in_motion, coord);
    const bool has_moved = any(greaterThan(abs(motion), inv_size));
    const vec2 uv = gl_FragCoord.xy * inv_size;
    const vec2 prev_uv = uv + motion;

    const vec2 prev_motion = texture(in_prev_motion, prev_uv).xy;

    const vec3 current = texelFetch(in_color, coord, 0).rgb;
    vec3 prev = current;


    bool sample_valid = true;


    // Motion rejection
    if((flags & motion_rejection_bit) != 0) {
        const vec2 ratio = vec2(1.0, y_ratio);
        sample_valid = sample_valid && (length((prev_motion - motion) * ratio) < (length(motion * ratio) * 0.2 + 0.05));
    }


    // Clamping
    if(sample_valid) {
        prev = texture(in_prev, prev_uv).rgb;

        if((flags & clamping_bit) != 0) {
            const ClampingInfo clamping_info = gather_clamping_info(in_color, coord);
            const vec3 clamped = clamp(prev, clamping_info.min_color, clamping_info.max_color);
            prev = mix(clamped, prev, 0.5); // ???
        }
    }





    if(sample_valid) {
        out_color = vec4(mix(current, prev, blending_factor), 1.0);
    } else {
        out_color = vec4(current, 1.0);
    }

#if 0
    if(!sample_valid) {
        out_color = vec4(1, 0, 0, 1);
    }
#endif
}


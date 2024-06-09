#version 460

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
    uint weighting_mode;
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
            const vec3 smp = max(vec3(0.0), texelFetch(color, coord + ivec2(x, y), 0).rgb);
            info.max_color = max(info.max_color, smp);
            info.min_color = min(info.min_color, smp);
        }
    }

    return info;
}




vec3 blend_weighted(vec3 curr, vec3 prev, float factor) {
    switch(weighting_mode) {
        case 0:     // None
            return mix(curr, prev, factor);

        case 1: {   // Lum
            const float curr_weight = (1.0 - factor) / luminance(curr);
            const float prev_weight = factor / luminance(prev);
            const float total_weight = curr_weight + prev_weight;
            return (curr * curr_weight + prev * prev_weight) / total_weight;
        }

        case 2: {   // Log
            const vec3 log_curr = log(max(curr, epsilon));
            const vec3 log_prev = log(max(prev, epsilon));
            return exp(mix(log_curr, log_prev, factor));
        }

        default:
        break;
    }

    return curr;
}

vec3 clamp_weighted(vec3 color, ClampingInfo info) {
    // Always use log
    const vec3 log_color = log(max(color, epsilon));
    const vec3 log_min = log(max(info.min_color, epsilon));
    const vec3 log_max = log(max(info.max_color, epsilon));
    return exp(clamp(log_color, log_min, log_max));
}




float filter_weight(float x) {
    const float B = 0.0;
    const float C = 0.5;

    const float x2 = x * x;
    const float x3 = x * x * x;

    float y = 0.0f;
    if(x < 1.0) {
        y = (12.0 - 9.0 * B - 6.0 * C) * x3 + (-18.0 + 12.0 * B + 6.0 * C) * x2 + (6.0 - 2.0 * B);
    } else if(x <= 2) {
        y = (-B - 6.0 * C) * x3 + (6.0 * B + 30.0 * C) * x2 + (-12.0 * B - 48.0 * C) * x + (8.0 * B + 24.0 * C);
    }

    return y / 6.0f;
}


// https://github.com/TheRealMJP/MSAAFilter/blob/master/MSAAFilter/Resolve.hlsl
vec3 sample_prev(vec2 prev_coord) {
    vec3 sum = vec3(0.0);
    float total = 0.0;
    for(int x = -1; x <= 2; ++x) {
        for(int y = -1; y <= 2; ++y) {
            const vec2 sample_coord = floor(prev_coord + vec2(x, y)) + 0.5;
            const vec3 prev_sample = texelFetch(in_prev, ivec2(sample_coord), 0).rgb;

            const vec2 sample_delta = abs(sample_coord - prev_coord);
            const float weight = filter_weight(sample_delta.x) * filter_weight(sample_delta.y);

            sum += prev_sample * weight;
            total += weight;
        }
    }

    return sum / total;
}


// -------------------------------- Main --------------------------------

const uint clamping_bit                  = 0x01;
const uint motion_rejection_bit          = 0x02;
const uint match_prev_bit                = 0x04;
const uint weighted_clamp_bit            = 0x08;


// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
void main() {
    const ivec2 coord = ivec2(gl_FragCoord.xy);

    const vec2 size = vec2(textureSize(in_color, 0).xy);
    const vec2 inv_size = 1.0 / size;
    const float y_ratio = size.y * inv_size.x;

    const vec2 motion = texelFetch(in_motion, coord, 0).xy;
    const bool has_moved = any(greaterThan(abs(motion), inv_size));
    const vec2 uv = gl_FragCoord.xy * inv_size;
    const vec2 prev_uv = uv + motion;

    const vec3 current = texelFetch(in_color, coord, 0).rgb;
    vec3 prev = current;


    bool sample_valid = saturate(prev_uv) == prev_uv;


    // Motion rejection
    if(sample_valid && (flags & motion_rejection_bit) != 0) {
        const vec2 ratio = vec2(1.0, y_ratio);
        const vec2 prev_motion = texture(in_prev_motion, prev_uv).xy;
        sample_valid = (length((prev_motion - motion) * ratio) < (length(motion * ratio) * 0.2 + 0.05));
    }


    // Clamping
    if(sample_valid) {
        if((flags & match_prev_bit) != 0) {
            prev = sample_prev(prev_uv * size);
        } else {
            prev = texture(in_prev, prev_uv).rgb;
        }

        if((flags & clamping_bit) != 0) {
            const ClampingInfo clamping_info = gather_clamping_info(in_color, coord);
            if((flags & weighted_clamp_bit) != 0) {
                prev = clamp_weighted(prev, clamping_info);
            } else {
                prev = clamp(prev, clamping_info.min_color, clamping_info.max_color);
            };
        }
    }




    if(sample_valid) {
        out_color = vec4(blend_weighted(current, prev, blending_factor), 1.0);
    } else {
        out_color = vec4(current, 1.0);
    }

#if 0
    if(!sample_valid) {
        out_color = vec4(1, 0, 0, 1);
    }
#endif
}


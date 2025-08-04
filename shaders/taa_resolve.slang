#include "lib/utils.slang"


// -------------------------------- I/O --------------------------------


[[vk::binding(0)]]
Sampler2D in_color;

[[vk::binding(1)]]
Sampler2D in_prev_color;

[[vk::binding(2)]]
Sampler2D<float> in_depth;

[[vk::binding(3)]]
Sampler2D<float2> in_motion;

[[vk::binding(4)]]
Sampler2D<uint> in_mask;

[[vk::binding(5)]]
ConstantBuffer<Camera> camera;

[[vk::binding(6)]]
cbuffer Settings_Inline {
    TAAFeatureBits flags;
    uint weighting_mode;
    float blending_factor;
};




// -------------------------------- Utils --------------------------------

struct ClampingInfo {
    float3 max_color;
    float3 min_color;
};

ClampingInfo gather_clamping_info(Sampler2D color, int2 coord) {
    ClampingInfo info;
    info.max_color = float3(0.0);
    info.min_color = float3(999999.0);

    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const float3 smp = max(float3(0.0), color[coord + int2(x, y)].rgb);
            info.max_color = max(info.max_color, smp);
            info.min_color = min(info.min_color, smp);
        }
    }

    return info;
}

float3 blend_weighted(float3 curr, float3 prev, float factor) {
    switch(weighting_mode) {
        case 0:     // None
            return lerp(curr, prev, factor);

        case 1: {   // Lum
            const float curr_weight = (1.0 - factor) / luminance(curr);
            const float prev_weight = factor / luminance(prev);
            const float total_weight = curr_weight + prev_weight;
            return (curr * curr_weight + prev * prev_weight) / total_weight;
        }

        case 2: {   // Log
            const float3 log_curr = log(max(curr, epsilon));
            const float3 log_prev = log(max(prev, epsilon));
            return exp(lerp(log_curr, log_prev, factor));
        }

        default:
        break;
    }

    return curr;
}

float3 clamp_weighted(float3 color, ClampingInfo info) {
    // Always use log
    const float3 log_color = log(max(color, epsilon));
    const float3 log_min = log(max(info.min_color, epsilon));
    const float3 log_max = log(max(info.max_color, epsilon));
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
float3 sample_history(float2 prev_coord) {
    float3 sum = float3(0.0);
    float total = 0.0;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const float2 sample_coord = floor(prev_coord + float2(x, y)) + 0.5;
            const float3 prev_sample = in_prev_color[int2(sample_coord)].rgb;

            const float2 sample_delta = abs(sample_coord - prev_coord);
            const float weight = filter_weight(sample_delta.x) * filter_weight(sample_delta.y);

            sum += prev_sample * weight;
            total += weight;
        }
    }

    return sum / total;
}

float2 sample_closest_velocity(int2 coord) {
#if 0
    int2 closest_pos = coord;
    float closest_depth = 0.0;
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            const int2 sample_pos = coord + int2(x, y);
            const float depth = in_depth[sample_pos];
            if (depth > closest_depth) {
                closest_depth = depth;
                closest_pos = sample_pos;
            }
        }
    }

    return in_motion[closest_pos];
#else
    return in_motion[coord];
#endif
}

float2 camera_reproject(float2 uv) {
    const float3 w = unproject(uv, 0.000001, camera.cur.inv_view_proj);
    const float2 reproj = project(w, camera.prev.view_proj).xy;
    return (reproj - uv) - camera.combined_jitter * 0.5;
}


struct PixelStageIn {
    float3 screen_pos;
    float3 last_screen_pos;
};

struct VertexStageOut {
    float4 sv_position : SV_Position;
    PixelStageIn frag;

};

float2 camera_reproject(PixelStageIn in) {
    return ((in.last_screen_pos.xy / in.last_screen_pos.z) - (in.screen_pos.xy / in.screen_pos.z) - camera.combined_jitter) * 0.5;
}




// -------------------------------- Main --------------------------------
[shader("vertex")]
VertexStageOut vert_main() {
    static const float2 uvs[] = {
        float2(0.0, 0.0),
        float2(0.0, 2.0),
        float2(2.0, 0.0),
    };

    const float2 uv = uvs[semantics.vertex_index];

    const float3 unproj = unproject(uv, 0.000001, camera.cur.inv_view_proj);
    const float4 current_position = mul(camera.cur.view_proj, float4(unproj, 1.0));
    const float4 last_position = mul(camera.prev.view_proj, float4(unproj, 1.0));

    VertexStageOut out;
    {
        out.sv_position = float4((uv * 2.0 - 1.0), 0.0, 1.0);
        out.frag.screen_pos = current_position.xyw;
        out.frag.last_screen_pos = last_position.xyw;
    }
    return out;
}


// https://www.elopezr.com/temporal-aa-and-the-quest-for-the-holy-trail/
[shader("fragment")]
float4 frag_main(PixelStageIn in) : SV_Target0 {
    const int2 coord = int2(semantics.frag_coord.xy);

    const float3 current = in_color[coord].rgb;

    const uint mask = in_mask[coord];
    if(mask != TAAMask::None && mask != TAAMask::DepthOutOfBounds) {
        return float4(current, 1.0);
    }

    const float2 size = float2(image_size(in_color).xy);
    const float2 uv = semantics.frag_coord.xy / size;

    float2 motion = mask == TAAMask::DepthOutOfBounds ? camera_reproject(in) : sample_closest_velocity(coord);
    const float2 prev_uv = uv + motion;

    float3 prev = current;

    if((flags & TAAFeatureBits::MatchPrevBit) != 0) {
        prev = sample_history(prev_uv * size);
    } else {
        prev = in_prev_color.Sample(prev_uv).rgb;
    }

    if((flags & TAAFeatureBits::ClampingBit) != 0) {
        const ClampingInfo clamping_info = gather_clamping_info(in_color, coord);
        if((flags & TAAFeatureBits::WeightedClampBit) != 0) {
            prev = clamp_weighted(prev, clamping_info);
        } else {
            prev = clamp(prev, clamping_info.min_color, clamping_info.max_color);
        }
    }

    return float4(blend_weighted(current, prev, blending_factor), 1.0);
}


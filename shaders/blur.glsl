layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform Weights_Inline {
    vec4 weights[4];
};


layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;



vec4 blur(sampler2D tex, vec2 uv, vec2 offset) {
    // float total_weight = weights[0][0];
    vec4 total = texture(tex, uv) * weights[0][0];;
    for(int i = 1; i != 11; ++i) {
        const vec2 o = offset * i;
        const float w = weights[i / 4][i % 4];
        total += texture(tex, uv + o) * w;
        total += texture(tex, uv - o) * w;
        //total_weight += 2.0 * w;
    }

    return total;
}


vec2 vertical_blur_offset(sampler2D tex, float scale) {
    return vec2(0.0, scale / textureSize(tex, 0).y);
}

vec2 horizontal_blur_offset(sampler2D tex, float scale) {
    return vec2(scale / textureSize(tex, 0).x, 0.0);
}

void main() {
#ifdef H_BLUR
    const vec2 offset = horizontal_blur_offset(in_color, 1.0);
#else
    const vec2 offset = vertical_blur_offset(in_color, 1.0);
#endif

    out_color = blur(in_color, in_uv, offset);
}



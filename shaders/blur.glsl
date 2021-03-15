layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform Weights_Inline {
    vec4 weights[3];
};

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;


vec4 blur(sampler2D tex, vec2 uv) {
    const vec2 inv_scale = 1.0 / textureSize(tex, 0).xy;
    vec4 total = texture(tex, uv) * weights[0][0];
    for(int i = 1; i != 12; ++i) {
        vec2 offset = vec2(0.0, 0.0);
        offset.BLUR_DIRECTION = (inv_scale.BLUR_DIRECTION * i);
        const float w = weights[i / 4][i % 4];
        total += texture(tex, uv + offset) * w;
        total += texture(tex, uv - offset) * w;
    }

    return total;
}

void main() {
    out_color = blur(in_color, in_uv);
}



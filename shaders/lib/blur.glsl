#ifndef BLUR_GLSL
#define BLUR_GLSL

vec2 vertical_blur_offset(sampler2D tex, float scale) {
    return vec2(0.0, scale / textureSize(tex, 0).y);
}

vec2 horizontal_blur_offset(sampler2D tex, float scale) {
    return vec2(scale / textureSize(tex, 0).x, 0.0);
}

vec4 blur(sampler2D tex, vec2 uv, vec2 offset) {
    /*float weights[5];
    weights[0] = 0.227027;
    weights[1] = 0.1945946;
    weights[2] = 0.1216216;
    weights[3] = 0.054054;
    weights[4] = 0.016216;*/

    float weights[11];
    weights[0] = 0.061476;
    weights[1] = 0.060998;
    weights[2] = 0.059587;
    weights[3] = 0.057307;
    weights[4] = 0.054261;
    weights[5] = 0.050582;
    weights[6] = 0.046421;
    weights[7] = 0.041944;
    weights[8] = 0.037311;
    weights[9] = 0.032676;
    weights[10] = 0.028174;

    vec4 total = texture(tex, uv) * weights[0];
    for(int i = 1; i != 11; ++i) {
        const vec2 o = offset * i;
        total += texture(tex, uv + o) * weights[i];
        total += texture(tex, uv - o) * weights[i];
    }

    return total;
}

#endif


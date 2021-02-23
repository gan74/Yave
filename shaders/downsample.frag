#version 450

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

#define FETCH_COMPONENT(i) samples[i] = textureGather(in_color, in_uv, i); avg[i] = dot(samples[i], vec4(0.25))

void main() {
    vec4 avg;
    mat4 samples;

    FETCH_COMPONENT(0);
    FETCH_COMPONENT(1);
    FETCH_COMPONENT(2);
    FETCH_COMPONENT(3);

    samples = transpose(samples);

    uint index = 0;
    float best = abs(dot(samples[0], avg));
    for(uint i = 1; i != 4; ++i) {
        const float score = abs(dot(samples[i], avg));
        if(score < best) {
            best = score;
            index = i;
        }
    }

    out_color = samples[index];
}


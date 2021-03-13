#version 450

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;
layout(set = 0, binding = 1) uniform BloomParams_Inline {
    float power;
    float threshold;
    float rev_threshold;
};

layout(location = 0) in vec2 in_uv;


void main() {
    const vec3 color = texture(in_color, in_uv).rgb;

    const vec3 thresholded = (color - threshold) * rev_threshold;
    out_color = vec4(pow(thresholded, vec3(power)), 1.0);
}


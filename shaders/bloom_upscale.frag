#version 460

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(set = 0, binding = 1) uniform BloomParams_Inline {
    vec2 filter_size;
    float alpha;
};

layout(location = 0) in vec2 in_uv;

void main() {
    const float x = filter_size.x;
    const float y = filter_size.y;

    const vec3 a = texture(in_color, in_uv + vec2( -x,   y)).rgb;
    const vec3 b = texture(in_color, in_uv + vec2(0.0,   y)).rgb;
    const vec3 c = texture(in_color, in_uv + vec2(  x,   y)).rgb;
    const vec3 d = texture(in_color, in_uv + vec2( -x, 0.0)).rgb;
    const vec3 e = texture(in_color, in_uv).rgb;
    const vec3 f = texture(in_color, in_uv + vec2(  x, 0.0)).rgb;
    const vec3 g = texture(in_color, in_uv + vec2( -x,  -y)).rgb;
    const vec3 h = texture(in_color, in_uv + vec2(0.0,  -y)).rgb;
    const vec3 i = texture(in_color, in_uv + vec2(  x,  -y)).rgb;

    out_color = vec4((
        e * 4.0 +
        (b + d + f + h) * 2.0 +
        (a + c + g + i)
    ) / 16.0, alpha);
}


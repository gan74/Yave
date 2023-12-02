#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(set = 0, binding = 0) uniform sampler2D font_texture;



float sampling_factor(sampler2D tex, vec2 uv) {
    const vec2 size = textureSize(tex, 0).xy;
    const vec2 coord = uv * size;

    const float x_factor = max(abs(dFdx(coord).x), abs(dFdx(coord).y));
    const float y_factor = max(abs(dFdy(coord).x), abs(dFdy(coord).y));

    return max(x_factor, y_factor);
}

vec4 texture_min_filter(sampler2D tex, vec2 uv) {
    const vec2 size = textureSize(tex, 0).xy;
    const vec2 inv_size = 1.0 / size;

    return (
        texture(tex, uv + inv_size * vec2( 1.0,  1.0)) +
        texture(tex, uv + inv_size * vec2( 1.0, -1.0)) +
        texture(tex, uv + inv_size * vec2( 1.0, -1.0)) +
        texture(tex, uv + inv_size * vec2(-1.0, -1.0))
    ) * 0.25;
}

void main() {
    vec4 color = v_color;

#if 1
    if(sampling_factor(font_texture, v_uv) > 2.0) {
        color *= texture_min_filter(font_texture, v_uv);
    } else {
        color *= texture(font_texture, v_uv);
    }
#else
    color *= texture(font_texture, v_uv);
#endif

    // We can't do linear to sRGB conversion here, since this shader is used to render
    // images that are already in sRGB in the UI
    out_color = color;
}


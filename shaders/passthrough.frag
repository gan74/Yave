#version 450

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_color;

layout(location = 0) in vec2 in_uv;

void main() {
    const vec4 color = texture(in_color, in_uv);
    out_color = color;

    /*const vec3 weights = vec3(0.077847, 0.123317, 0.195346);

    vec4 color = vec4(0.0);
    color += textureOffset(in_color, in_uv, ivec2(-1, -1)) * weights.x;
    color += textureOffset(in_color, in_uv, ivec2(-1,  0)) * weights.y;
    color += textureOffset(in_color, in_uv, ivec2(-1,  1)) * weights.x;

    color += textureOffset(in_color, in_uv, ivec2( 0, -1)) * weights.y;
    color += textureOffset(in_color, in_uv, ivec2( 0,  0)) * weights.z;
    color += textureOffset(in_color, in_uv, ivec2( 0,  1)) * weights.y;

    color += textureOffset(in_color, in_uv, ivec2( 1, -1)) * weights.x;
    color += textureOffset(in_color, in_uv, ivec2( 1,  0)) * weights.y;
    color += textureOffset(in_color, in_uv, ivec2( 1,  1)) * weights.x;

    const float total = weights.x * 4 + weights.y * 4 + weights.z;
    out_color = color / total;*/

    /*vec4 color = vec4(0.0);
    color += textureOffset(in_color, in_uv, ivec2(-1, -1));
    color += textureOffset(in_color, in_uv, ivec2(-1,  0));
    color += textureOffset(in_color, in_uv, ivec2(-1,  1));

    color += textureOffset(in_color, in_uv, ivec2( 0, -1));
    color += textureOffset(in_color, in_uv, ivec2( 0,  0));
    color += textureOffset(in_color, in_uv, ivec2( 0,  1));

    color += textureOffset(in_color, in_uv, ivec2( 1, -1));
    color += textureOffset(in_color, in_uv, ivec2( 1,  0));
    color += textureOffset(in_color, in_uv, ivec2( 1,  1));

    out_color = color / 9.0;*/
}


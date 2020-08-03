#version 450

layout(location = 0) out vec2 out_uv;

vec2 uvs[] = {
        vec2(0.0, 0.0),
        vec2(0.0, 2.0),
        vec2(2.0, 0.0),
    };

void main() {
    out_uv = uvs[gl_VertexIndex];
    gl_Position = vec4((uvs[gl_VertexIndex] * 2.0 - 1.0), 0.0, 1.0);
}


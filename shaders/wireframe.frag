#version 460

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec4 v_color;

void main() {
    out_color = v_color;
}


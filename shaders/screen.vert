#version 450

out gl_PerVertex {
	vec4 gl_Position;
};

layout(location = 0) out vec2 v_uv;

vec2 uvs[] = {
		vec2(0.0, 0.0),
		vec2(0.0, 1.0),
		vec2(1.0, 1.0),

		vec2(0.0, 0.0),
		vec2(1.0, 1.0),
		vec2(1.0, 0.0)
	};

void main() {
	v_uv = uvs[gl_VertexIndex];
	gl_Position = vec4(uvs[gl_VertexIndex] * 2.0 - 1.0, 0.0, 1.0);
}

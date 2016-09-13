#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
 

layout(location = 0) in vec3 v_normals[3];
layout(location = 1) in vec2 v_uvs[3];

layout(location = 0) out vec3 o_normal;
layout(location = 1) out vec2 o_uv;
layout(location = 2) out vec3 o_barycentric;

void main(){
	vec3 barycentrics[3] = vec3[3](vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1));
	for(int i = 0; i < gl_in.length(); i++) {
		gl_Position = gl_in[i].gl_Position;
		
		o_normal = v_normals[i];
		o_uv = v_uvs[i];
		o_barycentric = barycentrics[i];

		EmitVertex();
	}
}
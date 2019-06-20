#version 450

layout(location = 0) out uint out_color;

layout(location = 0) in flat uint instance_id;

void main() {
	out_color = instance_id;
}

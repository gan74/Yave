#version 450

layout(location = 0) out uint out_color;

layout(location = 0) in flat uint in_instance_id;

void main() {
	out_color = in_instance_id;
}

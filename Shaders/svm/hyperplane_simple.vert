#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec4 in_normalDistance;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 normalDistance; /*	normal (xyz), distance (w)	*/
layout(location = 1) out vec4 color;

void main() {

	gl_Position = vec4(Vertex, 1);
	/*	*/
	normalDistance = in_normalDistance;
	color = vec4(gl_VertexID, 0, 0, 0);
}
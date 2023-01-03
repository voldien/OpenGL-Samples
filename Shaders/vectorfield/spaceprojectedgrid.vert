#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 0) out vec3 vertex;

void main() {
	gl_Position = vec4(Vertex, 1.0);
	vertex = Vertex;
}

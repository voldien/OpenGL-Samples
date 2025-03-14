#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 0) out vec2 screenUV;

void main() {
	gl_Position = vec4(Vertex.x, Vertex.y, 0, 1.0);
	screenUV = (vec2(Vertex.x, Vertex.y) + vec2(1, 1)) / 2.0;
}

#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inVertex;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 vertexColor;

void main() {
	gl_Position = vec4(inVertex, 0.0, 1.0);
	vertexColor = inColor;
}

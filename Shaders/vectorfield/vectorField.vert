#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inVertex;
layout(location = 1) in vec3 inForce;

layout(location = 0) out smooth vec3 outForce;

void main() {
	gl_Position = vec4(inVertex.xyz, 1);
	outForce = inForce;
}
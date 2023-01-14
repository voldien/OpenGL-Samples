#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 Vertex;
layout(location = 1) in vec4 Velocity;

layout(location = 0) out vec4 velocity;
layout(location = 2) out float ageTime;

#include "base.glsl"

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	*/
	float deltaTime;

	particle_setting setting;
	motion_t motion;

	vec4 color;
}
ubo;

void main() {
	gl_Position = vec4(Vertex.xyz, 1.0);
	ageTime = Vertex.w;
	velocity = Velocity;
}

#version 460 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVelocity;

layout(location = 0) out vec4 velocity;

#include "particle_base.glsl"

void main() {

	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition.xyz, 1.0);
	gl_PointSize = (10000.0f / gl_Position.w) * inPosition.w;
	velocity = inVelocity;
}

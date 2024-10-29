#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 Vertex;
layout(location = 1) in vec4 Velocity;

layout(location = 0) out vec4 velocity;
layout(location = 2) out float ageTime;

#include "base.glsl"

void main() {
	gl_Position = vec4(Vertex.xyz, 1.0);
	ageTime = Vertex.w;
	velocity = Velocity;
}

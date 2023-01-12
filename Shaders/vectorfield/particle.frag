#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) smooth in vec2 uv;
layout(location = 1) smooth in vec4 gColor;
layout(location = 2) in float ageTime;

/*  */
layout(binding = 0) uniform sampler2D tex0;

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

void main() { fragColor = texture(tex0, uv) * (ubo.color + gColor); }

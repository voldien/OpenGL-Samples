#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 1) in vec4 in_normalDistance;
layout(location = 2) in vec4 in_color;

layout(location = 0) out vec4 normalDistance; /*	normal (xyz), distance (w)	*/
layout(location = 1) out vec4 color;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	/*  */
	vec4 cameraPosition;
	vec2 scale;
}
ubo;

void main() {
	normalDistance = ubo.model * in_normalDistance;
	color = in_color;
}
#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 in_texturecoord;
layout(location = 1) in vec4 in_color;

layout(binding = 1) uniform sampler2D DiffuseTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	/*  */
	vec4 cameraPosition;
	vec2 scale;

	// Cutout
}
ubo;

void main() { fragColor = in_color; }
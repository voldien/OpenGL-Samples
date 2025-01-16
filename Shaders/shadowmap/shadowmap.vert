#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;

layout(location = 0) out vec2 OutTextureCoord;

#include "common.glsl"
#include "phongblinn.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 lightSpaceMatrix;

	/*	Light source.	*/
	DirectionalLight directional;
	Camera camera;

	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;

	float bias;
	float shadowStrength;
}
ubo;

void main() {
	gl_Position = ubo.lightSpaceMatrix * ubo.model * vec4(Vertex, 1.0);
	OutTextureCoord = TextureCoord;
}
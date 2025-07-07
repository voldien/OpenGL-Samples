#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 TextureCoord;

layout(location = 4) in int index;

layout(location = 0) out invariant flat int GIndex;
layout(location = 1) out vec2 OutTextureCoord;

#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;

	PointLight point_light[4];
	vec4 PCFFilters[32];
	float diskRadius;
	int samples;
}
ubo;

void main() {
	gl_Position = ubo.model * vec4(vertex, 1.0);
	GIndex = index;
	OutTextureCoord = TextureCoord;
}
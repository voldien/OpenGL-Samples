#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;

	vec4 tintColor;

	/*  */
	vec4 cameraPosition;
	vec2 scale;
	vec4 offset;
}
ubo;

void main() { gl_Position = ubo.model * vec4(Vertex, 1); }
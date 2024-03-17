#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec3 WorldPos_CS_in;
layout(location = 1) out vec3 Normal_CS_in;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Material	*/
	vec4 diffuseColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	vec4 gEyeWorldPos;
	float tessLevel;
}
ubo;

void main() {
	WorldPos_CS_in = (ubo.model * vec4(Vertex, 1.0)).xyz;
	Normal_CS_in = (ubo.model * vec4(Normal, 0.0)).xyz;
}
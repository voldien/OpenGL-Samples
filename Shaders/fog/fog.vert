#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec2 UV;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 tangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	vec4 fogColor;
	float CameraNear;
	float CameraFar;
	float fogStart;
	float fogEnd;
	float fogDensity;

	uint fogType;
	float fogItensity;
}
ubo;

void main() {
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);
	normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;
	UV = TextureCoord;
}
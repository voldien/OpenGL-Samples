#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 VertexPosition;
layout(location = 1) out vec2 UV;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;

layout(binding = 0) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 normalMatrix;

	vec3 gEyeWorldPos;
	float gDispFactor;

	/*	Light source.	*/
	vec3 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

void main() {
	VertexPosition = (ubo.model * vec4(Vertex, 1.0)).xyz;
	normal = (ubo.normalMatrix * vec4(Normal, 0.0)).xyz;
	tangent = (ubo.normalMatrix * vec4(Tangent, 0.0)).xyz;
	UV = TextureCoord;
}
#version 460

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 lightPos;
	vec4 ambientColor;
}
ubo;

void main() {
	vertex = (ubo.model * vec4(Vertex, 1.0)).xyz;
	normal = normalize((ubo.model * vec4(Normal, 0.0)).xyz);
	uv = TextureCoord;
}
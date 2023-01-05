#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

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
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	vec4 specularColor;
	vec4 viewPos;
	float shininess;
}
ubo;

layout(binding = 1, std140) uniform UniformInstanceBlock { mat4 model[64]; }
instance_ubo;

void main() {
	gl_Position = ubo.proj * ubo.view * instance_ubo.model[gl_InstanceIndex] * vec4(Vertex, 1.0);
	uv = TextureCoord;
	normal = normalize((instance_ubo.model[gl_InstanceIndex] * vec4(Normal, 0.0)).xyz);
}

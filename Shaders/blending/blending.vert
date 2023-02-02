#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec2 FragIN_uv;
layout(location = 1) out vec3 FragIN_normal;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Tint color.	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

void main() {
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);
	FragIN_normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	FragIN_uv = TextureCoord;
}
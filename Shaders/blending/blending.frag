#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 FragIN_uv;
layout(location = 1) in vec3 FragIN_normal;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;

void main() {

	/*	Compute directional light	*/
	vec4 lightColor = max(0.0, dot(normalize(FragIN_normal), -normalize(ubo.direction.xyz))) * ubo.lightColor;

	fragColor = texture(DiffuseTexture, FragIN_uv) * ubo.tintColor * (ubo.ambientColor + lightColor);
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_draw_instanced : enable
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(binding = 0) uniform sampler2D DiffuseTexture;

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

void main() {

	vec3 viewDir = normalize(ubo.viewPos.xyz - vertex);

	vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	float spec = pow(max(dot(normalize(normal), halfwayDir), 0.0), ubo.shininess);

	float contribution = max(dot(normalize(normal), normalize(ubo.direction.xyz)), 0.0);

	vec4 LightSpecular = ubo.specularColor * spec;
	vec4 LightColors = contribution * ubo.lightColor;

	fragColor = (ubo.ambientColor + LightColors + LightSpecular) * texture(DiffuseTexture, uv);
}
#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 lightDirection;
	vec4 lightColor;

	/*	*/
	vec4 ambientColor;
	vec4 specularColor;
	vec4 viewPos;

	float shininess;
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;

void main() {

	float spec = 0.0;

	vec3 viewDir = normalize(ubo.viewPos.xyz - vertex);

	float contribution = max(dot(normalize(normal), ubo.lightDirection.xyz), 0.0);

	vec4 lightColor = ubo.lightColor * contribution;

	vec3 halfwayDir = normalize(ubo.lightDirection.xyz + viewDir);
	spec = pow(max(dot(normalize(normal), halfwayDir), 0.0), ubo.shininess);

	vec4 pointLightSpecular = (ubo.specularColor * spec);
	pointLightSpecular.a = 1.0;
	lightColor.a = 1.0;

	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor + lightColor + pointLightSpecular);
}
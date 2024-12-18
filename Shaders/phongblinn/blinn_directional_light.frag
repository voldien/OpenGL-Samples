#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_control_flow_attributes : enable

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
	vec4 direction;
	vec4 lightColor;
	vec4 specularColor;
	vec4 ambientColor;
	vec4 viewDir;

	float shininess;
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;

void main() {

	const vec3 viewDir = ubo.viewDir.xyz;

	// Compute directional light
	vec4 pointLightSpecular = vec4(0);

	/*  Blinn	*/
	const vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	const float spec = pow(max(dot(normal, halfwayDir), 0.0), ubo.shininess);
	float contriubtion = max(0.0, dot(-normalize(ubo.direction.xyz), normalize(normal)));

	pointLightSpecular = (ubo.specularColor * spec);
	pointLightSpecular.a = 1.0;

	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor + ubo.lightColor * contriubtion + pointLightSpecular);
}
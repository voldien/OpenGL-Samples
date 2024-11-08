#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 ViewDir;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
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

#include "common.glsl"

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D NormalTexture;
layout(binding = 10) uniform sampler2D Irradiance;

void main() {

	const vec3 viewDir = ViewDir;

	// Compute directional light
	vec4 pointLightColors = vec4(0);
	vec4 pointLightSpecular = vec4(0);

	/*  Blinn	*/
	const vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	const float spec = pow(max(dot(normal, halfwayDir), 0.0), ubo.shininess);
	float contriubtion = max(0.0, dot(-normalize(ubo.direction.xyz), normalize(normal)));

	const vec2 irradiance_uv = inverse_equirectangular(normalize(normal));
	const vec4 irradiance_color = texture(Irradiance, irradiance_uv).rgba;

	pointLightSpecular = (ubo.specularColor * spec);
	pointLightColors.a = 1.0;

	fragColor = texture(DiffuseTexture, uv) *
				(ubo.ambientColor * irradiance_color + ubo.lightColor * contriubtion + pointLightSpecular);
}
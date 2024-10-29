#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(early_fragment_tests) in;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 lightSpace;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2DShadow ShadowTexture;
layout(binding = 10) uniform sampler2D Irradiance;

#include "common.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 lightSpaceMatrix;
	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec3 cameraPosition;

	float bias;
	float shadowStrength;
}
ubo;

float ShadowCalculation(const in vec4 fragPosLightSpace) {

	// perform perspective divide
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	const float bias = max(0.05 * (1.0 - dot(normalize(normal), -normalize(ubo.direction).xyz)), ubo.bias);
	projCoords.z *= (1 - bias);

	float shadow = textureProj(ShadowTexture, projCoords, 0).r;

	// if (projCoords.z > 1.0) {
	// 	shadow = 0.0;
	// }
	// if (fragPosLightSpace.w > 1) {
	// 	shadow = 0;
	// }

	return (1.0 - shadow);
}

void main() {
	vec3 viewDir = normalize(ubo.cameraPosition.xyz - vertex);

	vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	float spec = pow(max(dot(normalize(normal), halfwayDir), 0.0), 8);

	vec4 color = texture(DiffuseTexture, UV);
	float shadow = max(ubo.shadowStrength - ShadowCalculation(lightSpace), 0);

	float contriubtion = max(0.0, dot(-normalize(ubo.direction.xyz), normalize(normal)));

	const vec2 irradiance_uv = inverse_equirectangular(normalize(normal));
	const vec4 irradiance_color = texture(Irradiance, irradiance_uv).rgba;

	const vec4 lighting = (ubo.ambientColor * irradiance_color + (ubo.lightColor * contriubtion + spec) * shadow) * color;

	fragColor = lighting;
}
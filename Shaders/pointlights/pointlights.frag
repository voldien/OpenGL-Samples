#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

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
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 2) uniform sampler2D NormalTexture;

float computeLightContributionFactor(in vec3 direction, in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

void main() {

	// Create new normal per pixel based on the normal map.
	vec3 Mnormal = normalize(normal);
	vec3 Ttangent = normalize(tangent);
	Ttangent = normalize(Ttangent - dot(Ttangent, Mnormal) * Mnormal);
	vec3 bittagnet = cross(Ttangent, Mnormal);

	vec3 NormalMapBump = 2.0 * texture(NormalTexture, UV).xyz - vec3(1.0, 1.0, 1.0);

	vec3 alteredNormal = normalize(mat3(Ttangent, bittagnet, Mnormal) * NormalMapBump);

	// Compute directional light
	vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, alteredNormal) * ubo.lightColor;

	fragColor = texture(DiffuseTexture, UV) * ( ubo.ambientColor + lightColor);
}
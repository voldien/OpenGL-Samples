#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(early_fragment_tests) in;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 lightSpace;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2DShadow ShadowTexture;

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

#define EPSILON 0.00001

float ShadowCalculation(const in vec4 fragPosLightSpace) {

	// perform perspective divide
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	float bias = max(0.05 * (1.0 - dot(normalize(normal), -normalize(ubo.direction).xyz)), ubo.bias);
	projCoords.z *= (1 - bias);

	float shadowFactor = 0;

	float xOffset = 1.0 / gMapSize.x;
	float yOffset = 1.0 / gMapSize.y;

	const float nrSamples = 3 * 3;

	[[unroll]] for (int y = -1; y <= 1; y++) {
		[[unroll]] for (int x = -1; x <= 1; x++) {
			vec2 Offsets = vec2(x * xOffset, y * yOffset);
			vec3 UVC = vec3(projCoords.xy + Offsets, z + EPSILON);
			shadowFactor += texture(gShadowMap, UVC);
		}
	}

	return shadowFactor / nrSamples;
}

void main() {
	vec3 viewDir = normalize(ubo.cameraPosition.xyz - vertex);

	vec3 halfwayDir = normalize(ubo.direction.xyz + viewDir);
	float spec = pow(max(dot(normalize(normal), halfwayDir), 0.0), 8);

	vec4 color = texture(DiffuseTexture, UV);
	float shadow = max(ubo.shadowStrength - ShadowCalculation(lightSpace), 0);

	float contriubtion = max(0.0, dot(-normalize(ubo.direction.xyz), normalize(normal)));

	vec4 lighting = (ubo.ambientColor + (ubo.lightColor * contriubtion + spec) * shadow) * color;

	fragColor = lighting;
}
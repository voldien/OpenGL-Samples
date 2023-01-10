#version 460
#extension GL_ARB_separate_shader_objects : enable

/*  */
layout(location = 0) out vec4 fragColor;

/*  */
layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec2 UV;
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
	vec4 ambientColor;

	vec4 gEyeWorldPos;
	float gDispFactor;
	float tessLevel;
}
ubo;

/*  */
layout(binding = 1) uniform sampler2D diffuse;
layout(binding = 1) uniform sampler2D heightMap;

float computeLightContributionFactor(in vec3 direction, in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

void main() {

	/*	Compute directional light	*/
	vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, normal) * ubo.lightColor;

	fragColor = texture(diffuse, UV) * (ubo.ambientColor + lightColor);
}
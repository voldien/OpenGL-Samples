#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 UV;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;
layout(location = 3) in vec4 lightSpace;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D ShadowTexture;

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
	vec3 lightPosition;

	float bias;
	float shadowStrength;
}
ubo;

float ShadowCalculation(vec4 fragPosLightSpace) {
	// perform perspective divide
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;
	// get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
	float closestDepth = texture(ShadowTexture, projCoords.xy).r;
	// get depth of current fragment from light's perspective
	float currentDepth = projCoords.z;
	// check whether current frag pos is in shadow
	float bias = max(0.05 * (1.0 - dot(normal, ubo.direction.xyz)), ubo.bias);
	// float bias = 0.005;
	float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;

	if (projCoords.z > 1.0)
		shadow = 0.0;

	return shadow;
}

void main() {
	// vec3 viewDir = normalize(ubo.viewPos.xyz - vertex);
	//
	// vec3 halfwayDir = normalize(lightDir + viewDir);
	// spec = pow(max(dot(normal, halfwayDir), 0.0), ubo.shininess);

	vec4 color = texture(DiffuseTexture, UV);
	float shadow = ShadowCalculation(lightSpace) * ubo.shadowStrength;
	vec4 lighting = (ubo.ambientColor + (1.0 - shadow) * (ubo.lightColor /* + specular*/)) * color;

	fragColor = lighting;
}
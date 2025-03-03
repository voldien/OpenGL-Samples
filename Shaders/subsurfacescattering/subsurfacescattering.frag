#version 460
#extension GL_ARB_separate_shader_objects : enable

/*  */
layout(early_fragment_tests) in;
layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 lightSpace;

layout(binding = 0) uniform sampler2D DiffuseTexture;
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
	vec4 cameraPosition;
	vec4 subsurfaceColor;

	/*	*/
	float bias;
	float shadowStrength;
	float sigma;
	float range;
}
ubo;

float getExpToLinear(const in float nearZ, const in float farZ, const in float ndc_depth) {
	return (ndc_depth * (farZ - ndc_depth) + nearZ);
}

float trace() {

	vec4 projCoords = lightSpace / lightSpace.w;
	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	/*	Bias depending on the angle of the surface, to offset from the surface.	*/
	const float bias = max(0.05 * (1.0 - dot(normalize(normal), -normalize(ubo.direction).xyz)), ubo.bias);
	projCoords.z *= (1 - bias);

	/*	Get light distance and view distance.	*/
	const float d_i = texture(ShadowTexture, projCoords.xy).r;
	const float d_o = (lightSpace / lightSpace.w).z; //- bias;

	/*	*/
	const float volumeDistance = d_o - d_i;

	/*	*/
	return exp(-volumeDistance * ubo.sigma);
}

void main() {

	/*	*/
	const float contribution = max(dot(normalize(normal), -normalize(ubo.direction).xyz), 0.0);

	/*	*/
	const vec4 subsurface = trace() * ubo.subsurfaceColor;

	/*	*/
	const vec3 viewDir = normalize(ubo.cameraPosition.xyz - vertex);
	const vec3 halfwayDir = normalize(-normalize(ubo.direction).xyz + viewDir);
	const float spec = pow(max(dot(normalize(normal), halfwayDir), 0.0), 16);
	const vec4 pointLightSpecular = vec4(spec);

	const vec4 light = (subsurface + (contribution * ubo.lightColor) + ubo.ambientColor + pointLightSpecular);

	fragColor = texture(DiffuseTexture, UV) * light;
}
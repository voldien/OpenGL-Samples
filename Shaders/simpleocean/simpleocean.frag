#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

struct Wave {
	float wavelength;
	float amplitude;
	float speed;
	float steepness;
	vec2 direction;
	vec2 padding;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 lookDirection;
	vec4 direction;
	vec4 lightColor;
	vec4 specularColor;
	vec4 ambientColor;
	vec4 position;

	Wave waves[64];
	int nrWaves;
	float time;

	/*	Material	*/
	float shininess;
	float fresnelPower;
	vec4 oceanColor;
}
ubo;

layout(binding = 1) uniform sampler2D ReflectionTexture;
layout(binding = 2) uniform sampler2D NormalTexture;

float computeLightContributionFactor(in const vec3 direction, in const vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

vec2 inverse_equirectangular(const in vec3 direction) {
	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

void main() {

	// Create new normal per pixel based on the normal map.
	const vec3 Mnormal = normalize(normal);

	/*	Compute directional light	*/
	const vec4 lightColor = computeLightContributionFactor(ubo.direction.xyz, Mnormal) * ubo.lightColor;

	/*	*/
	const vec3 viewDir = normalize(ubo.position.xyz - vertex);
	const vec3 diffVertex = (ubo.position.xyz - vertex);
	const vec3 lightDir = normalize(diffVertex);
	const vec3 halfwayDir = normalize(lightDir + viewDir);
	const float spec = pow(max(dot(Mnormal, halfwayDir), 0.0), ubo.shininess);

	const vec4 specular = ubo.specularColor * spec;

	/*	*/
	const vec3 reflection = normalize(reflect(viewDir, Mnormal));
	const vec2 reflection_uv = inverse_equirectangular(reflection);

	/*	*/
	float fresnel = max(dot(Mnormal, viewDir), 0);
	fresnel = pow(fresnel, ubo.fresnelPower);

	const vec4 color = mix(ubo.oceanColor, texture(ReflectionTexture, reflection_uv), fresnel);
	fragColor = color * (ubo.ambientColor + lightColor + specular);
	fragColor.a = 1 - fresnel;
}
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

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
	vec4 ambientColor;
	vec4 position;
	float time;

}
ubo;

layout(binding = 1) uniform sampler2D ReflectionTexture;
layout(binding = 2) uniform sampler2D NormalTexture;

float computeLightContributionFactor(in vec3 direction, in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

vec3 equirectangular(vec2 xy) {
	vec2 tc = xy / vec2(2.0) - 0.5;
	vec2 thetaphi =
		((tc * 2.0) - vec2(1.0)) * vec2(3.1415926535897932384626433832795, 1.5707963267948966192313216916398);
	vec3 rayDirection = vec3(cos(thetaphi.y) * cos(thetaphi.x), sin(thetaphi.y), cos(thetaphi.y) * sin(thetaphi.x));
	return rayDirection;
}

vec2 inverse_equirectangular(vec3 direction) {
	const vec2 invAtan = vec2(0.1591, 0.3183);
	vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
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

	vec3 I = normalize(ubo.position.xyz - vertex);
	vec3 reflection = reflect(I,  alteredNormal);
	vec2 reflection_uv = inverse_equirectangular(reflection);

	// TODO light improvemetns

	fragColor = texture(ReflectionTexture, reflection_uv) * (ubo.ambientColor + lightColor);
}
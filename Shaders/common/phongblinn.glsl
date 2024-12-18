#ifndef _COMMON_BLINN_PHONG_LIGHT_
#define _COMMON_BLINN_PHONG_LIGHT_ 1
#include "light.glsl"

vec4 computeBlinnDirectional(const in DirectionalLight light, const in vec3 normal, const in vec3 viewDir,
						  const in float shininess, const in vec3 specularColor) {

	/*  Blinn	*/
	const vec3 halfwayDir = normalize(normalize(light.direction.xyz) + viewDir);
	const float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
	const float contribution = computeLightContributionFactor(normalize(light.direction.xyz), normal);

	vec4 pointLightSpecular;
	pointLightSpecular.xyz = (specularColor * spec);
	pointLightSpecular.a = 1;
	return pointLightSpecular + contribution * light.lightColor;
}

vec4 computePhongDirectional(const in DirectionalLight light, const in vec3 normal, const in vec3 viewDir,
							 const in float shininess, const in vec3 specularColor) {

	/*	*/
	const vec3 reflectDir = reflect(-normalize(light.direction.xyz), normal);
	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	const float contribution = computeLightContributionFactor(normalize(light.direction.xyz), normal);

	vec4 pointLightSpecular;
	pointLightSpecular.xyz = (specularColor * spec);
	pointLightSpecular.a = 1;

	return pointLightSpecular + contribution * light.lightColor;
}

#endif
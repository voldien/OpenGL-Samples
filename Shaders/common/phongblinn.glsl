#ifndef _COMMON_BLINN_PHONG_LIGHT_
#define _COMMON_BLINN_PHONG_LIGHT_ 1
#include "light.glsl"

struct phongblinn_material {
	vec4 specular_shinnines;
};

vec4 computeBlinnDirectional(const in DirectionalLight light, const in vec3 normal, const in vec3 viewDir,
							 const in float shininess, const in vec3 specularColor) {

	/*  Blinn	*/
	const vec3 halfwayDir = normalize(-normalize(light.direction.xyz) + viewDir);

	const float normal_halfway_contr = max(dot(normal, halfwayDir), 0.0);
	const float spec = pow(normal_halfway_contr, shininess);
	const float contribution = computeLightContributionFactor(normalize(light.direction.xyz), normal);

	vec4 specularLightColor;
	specularLightColor = vec4( (specularColor * spec), 1);
	return specularLightColor * contribution + (contribution * light.lightColor);
}

vec4 computePhongDirectional(const in DirectionalLight light, const in vec3 normal, const in vec3 viewDir,
							 const in float shininess, const in vec3 specularColor) {

	/*	*/
	const vec3 reflectDir = reflect(-normalize(light.direction.xyz), normal);

	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
	const float contribution = computeLightContributionFactor(normalize(light.direction.xyz), normal);

	vec4 specularLightColor;
	specularLightColor = vec4( (specularColor * spec), 1);

	return specularLightColor * contribution + contribution * light.lightColor;
}

vec4 computePhongPoint(const in PointLight light, const in vec3 normal, const in vec3 vertex, const in vec3 viewDir,
					   const in float shininess, const in vec3 specularColor) {

	/*	*/
	vec3 diffVertex = (light.position - vertex);

	/*	*/
	float dist = length(diffVertex);

	/*	*/
	float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * dist +
							   light.qudratic_attenuation * (dist * dist));

	float contribution = max(dot(normal, normalize(diffVertex)), 0.0);

	/*	*/
	vec4 pointLightColors = (attenuation * light.color * contribution * light.range * light.intensity);

	return vec4(pointLightColors.rgb, 1);
}

#endif
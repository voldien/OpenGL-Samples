#ifndef _COMMON_LIGHT_H_
#define _COMMON_LIGHT_H_ 1

struct ShadowLight {
	mat4 lightSpaceMatrix;
	vec4 shadow;	/*	Shadow, bias*/
};

struct DirectionalLight {
	ShadowLight lightShadow;
	vec4 direction;
	vec4 lightColor;
};

struct PointLight {
	ShadowLight lightShadow;
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

float computeLightContributionFactor(const in vec3 direction, const in vec3 normalInput) {
	/*	*/
	return max(0.0, dot(-direction, normalInput));
}

vec4 computePoint(
	const in PointLight light,
	const in vec3 normal,
	const in vec3 vertex,
	const in float shininess,
	const in vec3 specularColor
) {

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

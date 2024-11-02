#include "light.glsl"

vec4 computeBlinnSpecular(const in DirectionalLight light, const in vec3 normal, const in vec3 viewDir,
							 const in float shininess) {

	// Compute directional light
	vec4 pointLightColors = vec4(0);
	vec4 pointLightSpecular = vec4(0);

	/*  Blinn	*/
	const vec3 halfwayDir = normalize(light.direction.xyz + viewDir);
	const float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
	float contriubtion = max(0.0, dot(-normalize(light.direction.xyz), normalize(normal)));

//	vec4 pointLightSpecular = (ubo.specularColor * spec);

	return pointLightSpecular;
}

// vec4 computePhongDirectional(const in DirectionalLight light) {

// 	const vec3 viewDir = ubo.viewDir.xyz;

// 	/*	*/
// 	const vec3 reflectDir = reflect(-ubo.direction.xyz, normal);
// 	const float spec = pow(max(dot(viewDir, reflectDir), 0.0), ubo.shininess);
// 	float contriubtion = max(0.0, dot(-normalize(ubo.direction.xyz), normalize(normal)));

// 	vec4 pointLightSpecular = (ubo.specularColor * spec);

// 	return pointLightSpecular;
// }

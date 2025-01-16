#ifndef _COMMON_PBR_H_
#define _COMMON_PBR_H_ 1
#include "common.glsl"
#include "light.glsl"

vec3 fresnelSchlickRoughness(const in float cosTheta, const in vec3 F0, const in float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlick(const in vec3 F0, const in vec3 V, const in vec3 N) {
	const float cosTheta = max(dot(N, V), 0.0);
	const float power = 5.0;
	const float fresnel_ratio = pow(1.0 - cosTheta, power);
	return mix(F0, vec3(1.0), fresnel_ratio);
}

vec3 FresnelSteinberg(const in vec3 F0, const in vec3 V, const in vec3 N) { return vec3(0); }

#endif
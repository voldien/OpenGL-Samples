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

vec3 getNormalFromMap(const in sampler2D normalMap, const in vec2 TexCoords, const in vec3 WorldPos,
					  const in vec3 Normal) {

	const vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

	const vec3 Q1 = dFdx(WorldPos);
	const vec3 Q2 = dFdy(WorldPos);
	const vec2 st1 = dFdx(TexCoords);
	const vec2 st2 = dFdy(TexCoords);

	const vec3 N = normalize(Normal);
	const vec3 T = normalize(Q1 * st2.t - Q2 * st1.t);
	const vec3 B = -normalize(cross(N, T));
	const mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

// ----------------------------------------------------------------------------
float DistributionGGX(const in vec3 N, const in vec3 H, const in float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySchlickGGX(const in float NdotV, const in float roughness) {
	float r = (roughness + 1.0);
	float k = (r * r) / 8.0;

	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return nom / denom;
}

// ----------------------------------------------------------------------------
float GeometrySmith(const in vec3 N, const in vec3 V, const in vec3 L, const in float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

vec4 computePBRPoint(const in PointLight light, const in vec3 normal, const in vec3 vertex, const in float shininess,
					 const in vec3 specularColor) {
	// reflectance equation
	// // reflectance equation
	// vec3 Lo = vec3(0.0);
	// for (uint i = 0; i <= 0; ++i) {

	// 	// calculate per-light radiance
	// 	vec3 L = vec3(0); // normalize(ubo.lightsettings.point_light[i].position - WorldPos);
	// 	vec3 H = normalize(V + L);
	// 	float distance = 1; // length(ubo.lightsettings.point_light[i].position - WorldPos);
	// 	float attenuation = 1.0 / (distance * distance);
	// 	vec3 radiance = vec3(0); // ubo.lightsettings.point_light[i].color.rgb * attenuation;

	// 	// Cook-Torrance BRDF
	// 	float NDF = DistributionGGX(N, H, roughness);
	// 	float G = GeometrySmith(N, V, L, roughness);
	// 	const vec3 F = vec3(0); // fresnelSchlick(max(dot(H, V), 0.0), F0);

	// 	const vec3 numerator = NDF * G * F;
	// 	const float denominator =
	// 		4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
	// 	vec3 specular = numerator / denominator;

	// 	// kS is equal to Fresnel
	// 	vec3 kS = F;
	// 	// for energy conservation, the diffuse and specular light can't
	// 	// be above 1.0 (unless the surface emits light); to preserve this
	// 	// relationship the diffuse component (kD) should equal 1.0 - kS.
	// 	vec3 kD = vec3(1.0) - kS;
	// 	// multiply kD by the inverse metalness such that only non-metals
	// 	// have diffuse lighting, or a linear blend if partly metal (pure metals
	// 	// have no diffuse light).
	// 	kD *= 1.0 - metallic;

	// 	// scale light by NdotL
	// 	float NdotL = max(dot(N, L), 0.0);

	// 	// add to outgoing radiance Lo
	// 	Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the
	// 															// Fresnel (kS) so we won't multiply by kS again
	// }
	return vec4(0);
}

#endif
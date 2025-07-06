#ifndef _COMMON_PBR_H_
#define _COMMON_PBR_H_ 1

#include "common.glsl"
#include "light.glsl"

vec3 fresnelSchlick(const in float cosTheta, const in vec3 f0) {
	return f0 + (1.0 - f0) * pow(max(1 - cosTheta, 0.0), 5.0);
}

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
					  const in vec3 Normal, const float bumpiness) {

	vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;
	tangentNormal.xy *= bumpiness;

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
// Normal Distributions
//-----------------------------------------------------------------------------
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

// --------------------------------------------------------------------
//	Geometry Term Functions
//---------------------------------------------------------------------
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

/***************************************************/

vec4 computePBRPoint(const in PointLight light, const in vec3 normal, const in vec3 vertex, const in float shininess,
					 const in vec3 specularColor) {
	return vec4(0);
}

/*	Cook-Torrance specular BRDF	*/
vec3 computePBRDirectionLight(const in DirectionalLight light, const in vec3 ViewPixelDir, const in vec3 SurfaceNormal,
							  const in float roughness, const in float metallic, const in vec3 F0,
							  const in vec3 albedo) {

	const vec3 light_direction = normalize(-light.direction.xyz);
	const vec3 half_vector = normalize(ViewPixelDir + light_direction);

	const float attenuation = 1;
	const vec3 radiance = light.lightColor.rgb * attenuation; // aka Li

	// Normal Distribution term (D)
	float dTerm = DistributionGGX(SurfaceNormal, half_vector, roughness);

	/*	Fresnel term (F)	*/
	// Determines the ratio of light reflected vs. absorbed
	vec3 fTerm = FresnelSchlick(half_vector, ViewPixelDir, F0);

	/* Geometry term (G)	*/
	float gTerm = GeometrySmith(SurfaceNormal, ViewPixelDir, light_direction, roughness);

	vec3 numerator = dTerm * fTerm * gTerm;
	float denominator =
		4.0 * max(dot(ViewPixelDir, SurfaceNormal), 0.0) * max(dot(light_direction, SurfaceNormal), 0.0);

	// recall fTerm is the proportion of reflected light, so the result here is the specular
	vec3 specular = numerator / max(denominator, 0.001);

	vec3 kSpecular = fTerm;
	vec3 kDiffuse = vec3(1.0) - kSpecular;
	kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component

	// now calculate full Cook-Torrance with both diffuse + specular
	//
	// f_r = kd * f_lambert + ks * f_cook-torrance
	//
	// where f_lambert = c / pi

	vec3 diffuse = kDiffuse * albedo / PI;
	vec3 cookTorranceBrdf = diffuse + specular;
	float nDotL = max(dot(SurfaceNormal, light_direction), 0.0);

	return cookTorranceBrdf * radiance * nDotL;
}

#endif
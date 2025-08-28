#ifndef _COMMON_PBR_H_
#define _COMMON_PBR_H_ 1

#include "common.glsl"
#include "light.glsl"

// ----------------------------------------------------------------------------
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(in uint bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// ----------------------------------------------------------------------------
vec2 Hammersley(const in uint i, const in uint N) { return vec2(float(i) / float(N), RadicalInverse_VdC(i)); }

// ----------------------------------------------------------------------------
vec3 ImportanceSampleGGX(const in vec2 Xi, const in vec3 N, const in float roughness) {
	const float a = roughness * roughness;

	float phi = 2.0 * PI * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// from spherical coordinates to cartesian coordinates - halfway vector
	vec3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// from tangent-space H vector to world-space sample vector
	vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, N));
	vec3 bitangent = cross(N, tangent);

	vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

vec3 fresnelSchlick(const in float cosTheta, const in vec3 f0) {

	return f0 + (1.0 - f0) * pow(max(1 - cosTheta, 0.0), 5.0);
}

vec3 fresnelSchlickRoughness(const in float cosTheta, const in vec3 F0, const in float roughness) {

	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlick(const in vec3 F0, const in vec3 V, const in vec3 N) {
	const float cosTheta = max(dot(N, V), 0.0);
	return fresnelSchlick(cosTheta, F0);
}

vec3 FresnelSteinberg(const in vec3 F0, const in vec3 V, const in vec3 N) { return vec3(0); }

// ----------------------------------------------------------------------------
// Normal Distributions
//-----------------------------------------------------------------------------
float DistributionGGX(const in vec3 N, const in vec3 H, const in float roughness) {
	const float a = roughness * roughness;
	const float a2 = a * a;
	const float NdotH = max(dot(N, H), 0.0);
	const float NdotH2 = NdotH * NdotH;

	const float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return nom / denom;
}

// --------------------------------------------------------------------
//	Geometry Term Functions
//---------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness) {
	// note that we use a different k for IBL
	float a = roughness;
	float k = (a * a) / 2.0;
	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}

/***************************************************/
// TODO:
vec3 brdfRadiance(const vec3 light_direction, const vec3 half_vector, const in vec3 ViewPixelDir,
				  const in vec3 SurfaceNormal, const in float roughness, const in float metallic, const in vec3 F0,
				  const in vec3 albedo, const in vec3 radiance) {
	// Normal Distribution term (D)
	float dTerm = DistributionGGX(SurfaceNormal, half_vector, roughness);

	/*	Fresnel term (F)	*/
	// Determines the ratio of light reflected vs. absorbed
	// const vec3 fTerm = FresnelSchlick(half_vector, ViewPixelDir, F0);
	const vec3 fTerm = fresnelSchlick(max(dot(half_vector, ViewPixelDir), 0.0), F0);

	/* Geometry term (G)	*/
	float gTerm = GeometrySmith(SurfaceNormal, ViewPixelDir, light_direction, roughness);

	const vec3 numerator = dTerm * fTerm * gTerm;
	float denominator =
		4.0 * max(dot(ViewPixelDir, SurfaceNormal), 0.0) * max(dot(light_direction, SurfaceNormal), 0.0);

	// recall fTerm is the proportion of reflected light, so the result here is the specular
	const vec3 specular = numerator / (denominator + 0.0001);

	vec3 kSpecular = fTerm;
	vec3 kDiffuse = vec3(1.0) - kSpecular;
	kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component

	vec3 diffuse = kDiffuse * albedo / PI;
	vec3 cookTorranceBrdf = diffuse + specular;
	float nDotL = max(dot(SurfaceNormal, light_direction), 0.0);

	return cookTorranceBrdf * radiance * nDotL;
}

vec3 computePBRPoint(const in PointLight light, const in vec3 worldPosition, const in vec3 ViewPixelDir,
					 const in vec3 SurfaceNormal, const in float roughness, const in float metallic, const in vec3 F0,
					 const in vec3 albedo) {

	const vec3 light_direction = normalize(light.position.xyz - worldPosition);
	const vec3 half_vector = normalize(ViewPixelDir + light_direction);

	const float distance = length(light.position.xyz - worldPosition);
	const float attenuation = (1 / (distance * distance));
	const vec3 radiance = light.color.rgb * attenuation * light.range;

	// Normal Distribution term (D)
	float dTerm = DistributionGGX(SurfaceNormal, half_vector, roughness);

	/*	Fresnel term (F)	*/
	// Determines the ratio of light reflected vs. absorbed
	// const vec3 fTerm = FresnelSchlick(half_vector, ViewPixelDir, F0);
	const vec3 fTerm = fresnelSchlick(max(dot(half_vector, ViewPixelDir), 0.0), F0);

	/* Geometry term (G)	*/
	float gTerm = GeometrySmith(SurfaceNormal, ViewPixelDir, light_direction, roughness);

	const vec3 numerator = dTerm * fTerm * gTerm;
	float denominator =
		4.0 * max(dot(ViewPixelDir, SurfaceNormal), 0.0) * max(dot(light_direction, SurfaceNormal), 0.0);

	// recall fTerm is the proportion of reflected light, so the result here is the specular
	const vec3 specular = numerator / (denominator + 0.0001);

	vec3 kSpecular = fTerm;
	vec3 kDiffuse = vec3(1.0) - kSpecular;
	kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component

	vec3 diffuse = kDiffuse * albedo / PI;
	vec3 cookTorranceBrdf = diffuse + specular;
	float nDotL = max(dot(SurfaceNormal, light_direction), 0.0);

	return cookTorranceBrdf * radiance * nDotL;
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

	/* Geometry term (G)	*/
	float gTerm = GeometrySmith(SurfaceNormal, ViewPixelDir, light_direction, roughness);

	/*	Fresnel term (F)	*/
	// Determines the ratio of light reflected vs. absorbed
	//	vec3 fTerm = FresnelSchlick(half_vector, ViewPixelDir, F0);
	const vec3 fTerm = fresnelSchlick(max(dot(half_vector, ViewPixelDir), 0.0), F0);

	const vec3 numerator = dTerm * fTerm * gTerm;
	const float denominator =
		4.0 * max(dot(ViewPixelDir, SurfaceNormal), 0.0) * max(dot(light_direction, SurfaceNormal), 0.0);

	// recall fTerm is the proportion of reflected light, so the result here is the specular
	const vec3 specular = numerator / (denominator + 0.0001);

	vec3 kSpecular = fTerm;
	vec3 kDiffuse = vec3(1.0) - kSpecular;
	kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component

	const float nDotL = max(dot(SurfaceNormal, light_direction), 0.0);

	const vec3 diffuse = (kDiffuse * albedo) / PI;
	const vec3 cookTorranceBrdf = diffuse + specular;

	return cookTorranceBrdf * radiance * nDotL;
}

#endif

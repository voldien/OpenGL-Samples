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
// ----------------------------------------------------------------------------
// Fresnel Distributions
//-----------------------------------------------------------------------------

vec3 fresnelSchlick(const in float cosTheta, const in vec3 f0) {
	float f = pow(1.0 - cosTheta, 5.0);
	return f + f0 * (1.0 - f);
}

vec3 fresnelSchlick(const in float cosTheta, const in vec3 f0, const in float f90) {
	return f0 + (vec3(f90) - f0) * pow(1.0 - cosTheta, 5.0);
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
#define MEDIUMP_FLT_MAX 65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
float DistributionGGX(const in vec3 N, const in vec3 H, const in float roughness) {

	const float a = roughness * roughness;
	const float a2 = a * a;
	const float NdotH = max(dot(N, H), 0.0);
	const float NdotH2 = (NdotH * a2 - NdotH) * NdotH + 1.0; // NdotH * NdotH;

	const float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	const float distribution = nom / denom;
	return saturateMediump(distribution);
}

float D_GGX(float roughness, float NoH, const vec3 n, const vec3 h) {
	vec3 NxH = cross(n, h);
	float a = NoH * roughness;
	float k = roughness / (dot(NxH, NxH) + a * a);
	float d = k * k * PI_INVERSE;
	return saturateMediump(d);
}

// --------------------------------------------------------------------
//	Geometry Term Functions
//---------------------------------------------------------------------

float GeometrySchlickGGX(const in float NdotV, const in float roughness) {
	// note that we use a different k for IBL
	float a = roughness;
	float k = (a * a) / 2.0;
	float nom = NdotV;
	float denom = NdotV * (1.0 - k) + k;
	return nom / denom;
}

float GeometrySmithGGXCorrelated(const in vec3 N, const in vec3 V, const in vec3 L, const in float roughness) {

	const float NdotV = max(dot(N, V), 0.0);
	const float NdotL = max(dot(N, L), 0.0);

	const float a2 = roughness * roughness;
	const float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
	const float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);

	return 0.5 / (GGXV + GGXL);
}

float GeometrySmithGGXCorrelatedFast(const in vec3 N, const in vec3 V, const in vec3 L, const in float roughness) {

	const float NdotV = max(dot(N, V), 0.0);
	const float NdotL = max(dot(N, L), 0.0);

	const float a = roughness;
	const float GGXV = NdotL * (NdotV * (1.0 - a) + a);
	const float GGXL = NdotV * (NdotL * (1.0 - a) + a);
	return 0.5 / (GGXV + GGXL);
}

float GeometrySmith(const in vec3 N, const in vec3 V, const in vec3 L, const in float roughness) {

	const float NdotV = max(dot(N, V), 0.0);
	const float NdotL = max(dot(N, L), 0.0);

	float ggx2 = GeometrySchlickGGX(NdotV, roughness);
	float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	return ggx1 * ggx2;
}

float Fd_Lambert() { return PI_INVERSE; }
float Fd_OrenNayar(const in vec3 lightDirection, const in vec3 viewDirection, const in vec3 surfaceNormal,
				   const in float roughness, const in float albedo) {

	float LdotV = dot(lightDirection, viewDirection);
	float NdotL = dot(lightDirection, surfaceNormal);
	float NdotV = dot(surfaceNormal, viewDirection);

	float s = LdotV - NdotL * NdotV;
	float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));

	float sigma2 = roughness * roughness;
	float A = 1.0 + sigma2 * (albedo / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));
	float B = 0.45 * sigma2 / (sigma2 + 0.09);

	return albedo * max(0.0, NdotL) * (A + B * s / t) / PI;
}

///////////
vec3 multiscattering_energy() {
	return vec3(0);
	//	vec3 energyCompensation = 1.0 + f0 * (1.0 / dfg.y - 1.0);
	// Scale the specular lobe to account for multiscattering
	// Fr *= pixel.energyCompensation;
}

/***************************************************/
// TODO: fix correct name and optimize with optional parameters during compilation.
vec3 BSDF(const vec3 light_direction, const vec3 halfway_vector, const in vec3 ViewPixelDir,
		  const in vec3 SurfaceNormal, const in float roughness, const in float metallic, const in vec3 F0,
		  const in vec3 albedo) {

	const float NoV = abs(dot(SurfaceNormal, ViewPixelDir)) + 1e-5;
	const float NoL = clamp(dot(SurfaceNormal, light_direction), 0.0, 1.0);
	const float NoH = clamp(dot(SurfaceNormal, halfway_vector), 0.0, 1.0);
	const float LoH = clamp(dot(light_direction, halfway_vector), 0.0, 1.0);

	/*	Normal Distribution term (D)	*/
	float dTerm = DistributionGGX(SurfaceNormal, halfway_vector, roughness);
	/* Geometry term (G)	*/ // TODO: add improved one
	float gTerm = GeometrySmith(SurfaceNormal, ViewPixelDir, light_direction, roughness);
	/*	Fresnel term (F)	*/
	const vec3 fTerm = fresnelSchlick(max(dot(halfway_vector, ViewPixelDir), 0.0), F0);

	const float VoN = max(dot(ViewPixelDir, SurfaceNormal), 0.0);

	/*	*/
	const vec3 numerator = dTerm * fTerm * gTerm;
	const float denominator = 4.0 * VoN * max(dot(light_direction, SurfaceNormal), 0.0);

	// recall fTerm is the proportion of reflected light, so the result here is the specular
	const float saturedDenominator = max(denominator, 0.0001);
	const vec3 specular = numerator / saturedDenominator;

	vec3 kSpecular = fTerm;
	vec3 kDiffuse = vec3(1.0) - kSpecular;
	kDiffuse *= 1.0 - metallic; // metallic materials should have no diffuse component

	const vec3 diffuse = (kDiffuse * albedo) * Fd_Lambert();

	return diffuse + specular;
}

vec3 computePBRPoint(const in PointLight light, const in vec3 worldPosition, const in vec3 ViewPixelDir,
					 const in vec3 SurfaceNormal, const in float roughness, const in float metallic, const in vec3 F0,
					 const in vec3 albedo) {

	const vec3 light_direction = normalize(light.position.xyz - worldPosition);
	const vec3 halfway_vector = normalize(ViewPixelDir + light_direction);

	const float distance = length(light.position.xyz - worldPosition);
	const float attenuation = (1 / (distance * distance));
	const vec3 radiance = light.color.rgb * attenuation * light.range;

	const float nDotL = computeLightContributionFactor(SurfaceNormal, -light_direction);

	const vec3 cookTorranceBrdf =
		BSDF(light_direction, halfway_vector, ViewPixelDir, SurfaceNormal, roughness, metallic, F0, albedo);

	return cookTorranceBrdf * radiance * nDotL;
}

/*	Cook-Torrance specular BRDF	*/
vec3 computePBRDirectionLight(const in DirectionalLight light, const in vec3 ViewPixelDir, const in vec3 SurfaceNormal,
							  const in float roughness, const in float metallic, const in vec3 F0,
							  const in vec3 albedo) {

	const vec3 light_direction = -light.direction.xyz;
	const vec3 halfway_vector = normalize(ViewPixelDir + light_direction);

	const vec3 radiance = light.lightColor.rgb; // aka Li

	const float nDotL = computeLightContributionFactor(SurfaceNormal, light.direction.xyz);

	const vec3 cookTorranceBrdf =
		BSDF(light_direction, halfway_vector, ViewPixelDir, SurfaceNormal, roughness, metallic, F0, albedo);

	return cookTorranceBrdf * radiance * nDotL;
}

#endif

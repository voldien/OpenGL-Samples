#include "noise.glsl"
#include "fog.glsl"

// enum GBuffer : unsigned int {

// 	WorldSpace = 1,
// 	TextureCoordinate = 2,
// 	Albedo = 0,
// 	Normal = 3,
// 	Specular = 4, // Roughness
// 	Emission = 5,
// };

/*	Constants.	*/
#define PI 3.1415926535897932384626433832795
#define PI_HALF 1.5707963267948966192313216916398
layout(constant_id = 0) const float EPSILON = 0.00001;

struct Camera {
	float near;
	float far;
	float aspect;
	float fov;
	vec4 position;
	vec4 viewDir;
	vec4 position_size;
};

vec4 bump(const in sampler2D BumpTexture, const in vec2 uv, const in float dist) {

	const vec2 size = vec2(2.0, 0.0);
	const vec2 offset = 1.0 / textureSize(BumpTexture, 0);

	const vec2 offxy = vec2(offset.x, offset.y);
	const vec2 offzy = vec2(offset.x, offset.y);
	const vec2 offyx = vec2(offset.x, offset.y);
	const vec2 offyz = vec2(offset.x, offset.y);

	const float s11 = texture(BumpTexture, uv).x;
	const float s01 = texture(BumpTexture, uv + offxy).x;
	const float s21 = texture(BumpTexture, uv + offzy).x;
	const float s10 = texture(BumpTexture, uv + offyx).x;
	const float s12 = texture(BumpTexture, uv + offyz).x;
	vec3 va = vec3(size.x, size.y, s21 - s01);
	vec3 vb = vec3(size.y, size.x, s12 - s10);
	va = normalize(va);
	vb = normalize(vb);
	const vec4 normal = vec4(cross(va, vb) / 2 + 0.5, 1.0);

	return normal;
}


float rand(const in float seed) { return fract(sin(seed) * 100000.0); }

float rand(const in vec2 co) { return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453); }

vec3 equirectangular(const in vec2 xy) {
	const vec2 tc = xy / vec2(2.0) - 0.5;

	const vec2 thetaphi =
		((tc * 2.0) - vec2(1.0)) * vec2(3.1415926535897932384626433832795, 1.5707963267948966192313216916398);
	const vec3 rayDirection =
		vec3(cos(thetaphi.y) * cos(thetaphi.x), sin(thetaphi.y), cos(thetaphi.y) * sin(thetaphi.x));

	return rayDirection;
}

vec2 inverse_equirectangular(const in vec3 direction) {
	const vec2 invAtan = vec2(1.0 / (2 * PI), 1.0 / PI);
	vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
	uv *= invAtan;
	uv += 0.5;
	return uv;
}

vec3 fresnelSchlickRoughness(const in float cosTheta, const in vec3 F0, const in float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlick(const vec3 SpecularColor, const vec3 E, const vec3 H) {
	return SpecularColor + (1.0f - SpecularColor) * pow(1.0f - clamp(dot(E, H), 0, 1), 5);
}

/**
 *
 */
vec3 getTBN(const in vec3 InNormal, const in vec3 InTangent, const in sampler2D normalSampler, const in vec2 uv) {
	return ((mat3(normalize(InTangent - dot(InTangent, InNormal) * InNormal), cross(InTangent, InNormal), InNormal)) *
			((2.0f * vec3(texture(normalSampler, uv).rg, 1.0)) - 1.0f))
		.xyz;
}

/**
 *
 */
vec3 getTBN(const in vec3 InNormal, const in vec3 InTangent, const in vec3 normalSampler) {
	return ((mat3(normalize(InTangent - dot(InTangent, InNormal) * InNormal), cross(InTangent, InNormal), InNormal)) *
			((2.0f * normalSampler) - 1.0f))
		.xyz;
}

vec2 getParallax(const in sampler2D heightMap, const in vec2 uv, const in vec3 cameraDir, const in vec2 biasScale) {
	const float v = texture(heightMap, uv).r * biasScale.x - biasScale.y;
	return (uv + (cameraDir.xy * v)).xy;
}
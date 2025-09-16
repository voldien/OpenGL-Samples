#ifndef _COMMON_HEADER_
#define _COMMON_HEADER_ 1

#extension GL_EXT_control_flow_attributes : enable
#extension GL_EXT_control_flow_attributes2 : enable

#include "colorspace.glsl"
#include "math.glsl"
#include "noise.glsl"
#include "texture.glsl"
#include "transformation.glsl"

/*	Application constant.	*/
layout(constant_id = 0) const float EPSILON = 1.19209e-07;

struct Camera {
	float near;	  /*	*/
	float far;	  /*	*/
	float aspect; /*	*/
	float fov;	  /*	*/

	vec4 position;		/*	*/
	vec4 viewDir;		/*	*/
	vec4 position_size; /*	*/
	uvec4 screen_width_padding;

	mat4 view;
	mat4 viewInv;
	mat4 viewRot;
	mat4 viewProj;
	mat4 viewProjInv;
	mat4 proj;
	mat4 inverseProj;
};

struct FogSettings {
	/*	*/
	vec4 fogColor;
	/*	*/
	float CameraNear;
	float CameraFar;
	float fogStart;
	float fogEnd;

	/*	*/
	float fogDensity;
	uint fogType;
	float fogItensity;
	float fogHeight;
};

/*	*/
struct Frustum {
	vec4 planes[6];
};

struct DrawElementsIndirectCommand {
	uint count;
	uint instanceCount;
	uint firstIndex;
	uint baseVertex;
	uint baseInstance;
};

struct IndirectDrawArray {
	uint count;			/*  */
	uint instanceCount; /*  */
	uint first;			/*  */
	uint baseInstance;	/*  */
};

struct IndirectDispatchCommand {
	uint num_groups_x;
	uint num_groups_y;
	uint num_groups_z;
};

vec3 equirectangular(const in vec2 xy) {
	const vec2 tc = xy / vec2(2.0) - 0.5;

	const vec2 thetaphi = ((tc * 2.0) - vec2(1.0)) * vec2(PI, PI_HALF);
	const vec3 rayDirection =
		vec3(cos(thetaphi.y) * cos(thetaphi.x), sin(thetaphi.y), cos(thetaphi.y) * sin(thetaphi.x));

	return normalize(rayDirection);
}

const vec2 invAtan = vec2(1.0 / (2 * PI), 1.0 / PI);
vec2 inverse_equirectangular(const in vec3 direction) {

	vec2 uv = vec2(atan(direction.z, direction.x + EPSILON), asin(direction.y + EPSILON));

	uv *= invAtan;
	uv += 0.5;
	return uv;
}

/**
 *
 */
vec3 getTBN(const in vec3 InNormal, const in vec3 InTangent, const in sampler2D NormalTexture,
			const in float normalStrength, const in vec2 uv) {

	/*	*/
	vec3 tangentNormal = texture(NormalTexture, uv).xyz * 2.0 - 1.0;
	tangentNormal.xy *= normalStrength;

	tangentNormal = normalize(tangentNormal);

	const vec3 tangentSurfaceNormal =
		((mat3(normalize(InTangent - dot(InTangent, InNormal) * InNormal), cross(InTangent, InNormal), InNormal)) *
		 tangentNormal)
			.xyz;

	return tangentSurfaceNormal;
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

#define CLAMP01(T) clamp((T), 0.0, 1.0)
#define EASE(T) smoothstep(0.0, 1.0, (T))

vec3 CatmulRom(in float T, vec3 D, vec3 C, vec3 B, vec3 A) {
	return 0.5 * ((2.0 * B) + (-A + C) * T + (2.0 * A - 5.0 * B + 4.0 * C - D) * T * T +
				  (-A + 3.0 * B - 3.0 * C + D) * T * T * T);
}

vec3 ColorRampConstant(in float T, const in vec4[4] A, const in int num) {
	[[unroll]]
	for (uint i = 0; i < num; i++) {
		if (T < A[i].w) {
			return A[i].rgb;
		}
	}
}

vec3 ColorRampLinear(in float T, const in vec4[4] Ramp, const in int num) {

	// for (uuint i = 0; i < num; i++) {
	// 	if (T < A[i].w) {
	// 		// return A[i].rgb;
	// 	}
	// }
	vec4 A = Ramp[0];
	vec4 B = Ramp[1];
	vec4 C = Ramp[2];
	vec4 D = Ramp[3];

	// Distances =
	float AB = B.w - A.w;
	float BC = C.w - B.w;
	float CD = D.w - C.w;

	// Intervales :
	float iAB = CLAMP01((T - A.w) / AB);
	float iBC = CLAMP01((T - B.w) / BC);
	float iCD = CLAMP01((T - C.w) / CD);

	// Pondérations :
	float pA = 1.0 - iAB;
	float pB = iAB - iBC;
	float pC = iBC - iCD;
	float pD = iCD;

	return pA * A.xyz + pB * B.xyz + pC * C.xyz + pD * D.xyz;
}

vec3 ColorRamp_Smoothstep(in float T, vec4 A, in vec4 B, in vec4 C, in vec4 D) {
	// Distances =
	float AB = B.w - A.w;
	float BC = C.w - B.w;
	float CD = D.w - C.w;

	// Intervales :
	float iAB = CLAMP01((T - A.w) / AB);
	float iBC = CLAMP01((T - B.w) / BC);
	float iCD = CLAMP01((T - C.w) / CD);

	// Pondérations :
	vec4 p = vec4(1.0 - iAB, iAB - iBC, iBC - iCD, iCD);
	p = EASE(p);
	return p.x * A.xyz + p.y * B.xyz + p.z * C.xyz + p.w * D.xyz;
}

vec3 ColorRamp_BSpline(in float T, vec4 A, in vec4 B, in vec4 C, in vec4 D) {
	// Distances =
	float AB = B.w - A.w;
	float BC = C.w - B.w;
	float CD = D.w - C.w;

	// Intervales :
	float iAB = CLAMP01((T - A.w) / AB);
	float iBC = CLAMP01((T - B.w) / BC);
	float iCD = CLAMP01((T - C.w) / CD);

	// Pondérations :
	vec4 p = vec4(1.0 - iAB, iAB - iBC, iBC - iCD, iCD);
	vec3 cA = CatmulRom(p.x, A.xyz, A.xyz, B.xyz, C.xyz);
	vec3 cB = CatmulRom(p.y, A.xyz, B.xyz, C.xyz, D.xyz);
	vec3 cC = CatmulRom(p.z, B.xyz, C.xyz, D.xyz, D.xyz);
	vec3 cD = D.xyz;

	if (T < B.w)
		return cA.xyz;
	if (T < C.w)
		return cB.xyz;
	if (T < D.w)
		return cC.xyz;
	return cD.xyz;
}

float getGuas2D(const in float x, const in float y, const in float variance) {
	const float inverse_2pi = (1.0 / sqrt(2.0 * PI));

	return inverse_2pi * (1.0 / (variance)) * exp(-0.5 * (((x * x) + (y * y)) / (variance * variance)));
}

vec2 pixelate_screenUV(const in vec2 screenUV, const in float pixel_size, const in vec2 aspect_ratio) {
	return floor(screenUV * pixel_size * aspect_ratio) / (pixel_size * aspect_ratio);
}

vec2 sphere_uv_mapping(const in vec3 normalized_direction) { return inverse_equirectangular(normalized_direction); }

struct Sphere {
	vec4 position_radius;
};

float ray_sphere_intersect(const vec3 center, const float radius, const vec3 origin, const vec3 ray_dir) {
	vec3 tmp = origin - center;
	float t;
	/*	*/
	float a = dot(ray_dir, ray_dir);
	float b = 2.0 * dot(ray_dir, tmp);
	float c = dot(tmp, tmp) - (radius * radius);

	/*	*/
	const float discriminant = b * b - (4.0f * c * a);
	if (discriminant < 0.0) {
		return -1;
	}
	return (-b - sqrt(discriminant)) / (a * 2.0);
}

vec2 ray_sphere_intersect_samples(const vec3 center, const float radius, const vec3 origin, const vec3 ray_dir) {
	vec3 tmp = origin - center;
	float t;
	/*	*/
	float a = dot(ray_dir, ray_dir);
	float b = 2.0 * dot(ray_dir, tmp);
	float c = dot(tmp, tmp) - (radius * radius);

	/*	*/
	const float discriminant = b * b - (4.0f * c * a);
	if (discriminant < 0.0) {
		return vec2(-1.0, -1.0);
	}

	return vec2(-b - sqrt(discriminant), -b + sqrt(discriminant)) / (2.0 * a);
}

#endif

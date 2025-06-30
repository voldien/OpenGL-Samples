#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(binding = 0) uniform sampler2D ColorTexture;
layout(binding = 2) uniform sampler2D DepthTexture;
layout(binding = 6) uniform sampler2D DirectionalLightShadowMap;

#include "postprocessing_base.glsl"

layout(push_constant) uniform UniformBufferBlock {
	layout(offset = 0) int numSamples;
	layout(offset = 4) float _Density;
	layout(offset = 8) float _Decay;
	layout(offset = 12) float _Weight;
	layout(offset = 16) float _Exposure;
	layout(offset = 32) vec2 lightPosition;
	layout(offset = 48) vec4 color;
}
settings;

// This function will tell us if a certain point in world space coordinates is in light or shadow of the main light
float ShadowAtten(vec3 worldPosition) { return 0; }

// Unity already has a function that can reconstruct world space position from depth
vec3 GetWorldPos(const in vec2 uv, const in mat4 inverProj) {
	const float depth = texture(DirectionalLightShadowMap, uv).r;

	return calcViewPosition(uv, inverProj, depth);
}

// Mie scaterring approximated with Henyey-Greenstein phase function.
float ComputeScattering(float lightDotView) {
	// float result = 1.0f - _Scattering * _Scattering;
	// result /= (4.0f * PI * pow(1.0f + _Scattering * _Scattering - (2.0f * _Scattering) * lightDotView, 1.5f));
	// return result;
	return 0;
}

// standart hash
float random(vec2 p) { return fract(sin(dot(p, vec2(41, 289))) * 45758.5453) - 0.5; }
float random01(vec2 p) { return fract(sin(dot(p, vec2(41, 289))) * 45758.5453); }

// from Ronja https://www.ronja-tutorials.com/post/047-invlerp_remap/
float invLerp(float from, float to, float value) { return (value - from) / (to - from); }
float remap(float origFrom, float origTo, float targetFrom, float targetTo, float value) {
	float rel = invLerp(origFrom, origTo, value);
	return mix(targetFrom, targetTo, rel);
}

// this implementation is loosely based on http://www.alexandre-pestana.com/volumetric-lights/
// and https://fr.slideshare.net/BenjaminGlatzel/volumetric-lighting-for-many-lights-in-lords-of-the-fallen

// #define MIN_STEPS 25

void main() {
	// first we get the world space position of every pixel on screen
	//vec3 worldPos = GetWorldPos(i.uv);

	//// we find out our ray info, that depends on the distance to the camera
	//vec3 startPosition = _WorldSpaceCameraPos;
	//vec3 rayVector = worldPos - startPosition;
	//vec3 rayDirection = normalize(rayVector);
	//float rayLength = length(rayVector);

	//if (rayLength > _MaxDistance) {
	//	rayLength = _MaxDistance;
	//	worldPos = startPosition + rayDirection * rayLength;
	//}

	// We can limit the amount of steps for close objects
	//  steps= remap(0,_MaxDistance,MIN_STEPS,_Steps,rayLength);
	// or
	//  steps= remap(0,_MaxDistance,0,_Steps,rayLength);
	//  steps = max(steps,MIN_STEPS);

	//float stepLength = rayLength / _Steps;
	//vec3 step = rayDirection * stepLength;
//
	//// to eliminate banding we sample at diffent depths for every ray, this way we obfuscate the shadowmap patterns
	//float rayStartOffset = random01(i.uv) * stepLength * _JitterVolumetric / 100;
	//vec3 currentPosition = startPosition + rayStartOffset * rayDirection;
//
	//float accumFog = 0;
//
	//// we ask for the shadow map value at different depths, if the sample is in light we compute the contribution at
	//// that point and add it
	//for (float j = 0; j < _Steps - 1; j++) {
	//	float shadowMapValue = ShadowAtten(currentPosition);
//
	//	// if it is in light
	//	if (shadowMapValue > 0) {
	//		float kernelColor = ComputeScattering(dot(rayDirection, _SunDirection));
	//		accumFog += kernelColor;
	//	}
	//	currentPosition += step;
	//}
	//// we need the average value, so we divide between the amount of samples
	//accumFog /= _Steps;
//
	//return accumFog;
}
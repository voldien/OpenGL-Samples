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
layout(binding = 15) uniform sampler2D DirectionalLightShadowMap;

#include "light.glsl"
#include "postprocessing_base.glsl"

layout(push_constant) uniform UniformBufferBlock {
	layout(offset = 0) int numSamples;
	layout(offset = 4) float _Density;
	layout(offset = 8) float _Decay;
	layout(offset = 12) float _Weight;
	layout(offset = 16) float _Exposure;
	layout(offset = 32) vec3 lightPosition;
	layout(offset = 48) vec4 color;
} settings;


vec3 calcViewPosition(const in vec2 coords) {
	const float fragmentDepth = texture(DepthTexture, coords).r;
	return calcViewPosition(coords, constantCommon.constant.camera.inverseProj, fragmentDepth);
}

vec3 GetWorldPos(const in vec2 uv) {

	const vec3 viewPosition = calcViewPosition(uv);

	return view_to_world(constantCommon.constant.camera.viewInv, viewPosition, true).xyz;
}

float ShadowAtten(const in vec3 worldPosition) {

	vec4 fragPosLightSpace = LightUBO.light.directional[0].lightShadow.lightSpaceMatrix * vec4(worldPosition.xyz, 1);

	/*	perform perspective divide	*/
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

	/*	*/
	if(fragPosLightSpace.w > 1.0) {
		return 0;
	}

	/*	transform from NDC to Screen Space [0,1] range	*/
	projCoords = ndc_to_uv(projCoords);

	/*	*/
	const float currentDepth = projCoords.z;
	const float shadow = texture(DirectionalLightShadowMap, projCoords.xy).r;

	return currentDepth < shadow ? 1 : 0;
}

// Mie scaterring approximated with Henyey-Greenstein phase function.
float ComputeScattering(float lightDotView) {

	const float scattering_square = settings._Decay * settings._Decay;
	float result = 1.0 - scattering_square;
	result /= (4.0 * PI * pow(1.0 + scattering_square - (2.0 * settings._Decay) * lightDotView, 1.5));
	return result;
}

// standart hash
float random(vec2 p) {
	return fract(sin(dot(p, vec2(41, 289))) * 45758.5453) - 0.5;
}
float random01(vec2 p) {
	return fract(sin(dot(p, vec2(41, 289))) * 45758.5453);
}

// from Ronja https://www.ronja-tutorials.com/post/047-invlerp_remap/
float invLerp(float from, float to, float value) {
	return (value - from) / (to - from);
}
float remap(float origFrom, float origTo, float targetFrom, float targetTo, float value) {
	float rel = invLerp(origFrom, origTo, value);
	return mix(targetFrom, targetTo, rel);
}

#define MIN_STEPS 1

void main() {
	const float _MaxDistance = 250;

	/*	*/
	const vec3 worldPos = GetWorldPos(screenUV);

	// we find out our ray info, that depends on the distance to the camera
	vec3 startPosition = constantCommon.constant.camera.position.xyz;
	vec3 rayVector = worldPos - startPosition;

	const vec3 rayDirection = normalize(rayVector);
	const float rayLength = length(rayVector);

	const float stepLength = rayLength / settings.numSamples;
	const vec3 step = rayDirection * stepLength;

	// to eliminate banding we sample at diffent depths for every ray, this way we obfuscate the shadowmap patterns
	float _JitterVolumetric = settings._Weight;
	float rayStartOffset = random01(screenUV) * stepLength * _JitterVolumetric;
	vec3 currentPosition = startPosition + rayStartOffset * rayDirection;

	float accumScattering = 0;
	for(float j = 0; j < settings.numSamples; j++) {

		float shadowMapValue = ShadowAtten(currentPosition) * settings._Exposure;

		// if it is in light
		if(shadowMapValue > 0) {
			float kernelColor = ComputeScattering(dot(rayDirection, -normalize(LightUBO.light.directional[0].direction.xyz)));
			accumScattering += kernelColor;
		}

		currentPosition += step;
	}

	/*	*/
	accumScattering /= settings.numSamples;

	const vec4 color = texture(ColorTexture, screenUV);

	fragColor = color + accumScattering * settings.color;
}
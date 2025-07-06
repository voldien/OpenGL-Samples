#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision highp float;
precision mediump int;

layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

layout(binding = 2) uniform sampler2D DepthTexture;
layout(binding = 4) uniform sampler2D NormalRandomize;

#include "common.glsl"

layout(push_constant) uniform UniformBufferBlock {
	layout(offset = 0) int numSamples;
	layout(offset = 4) float _Density;
	layout(offset = 8) float _Decay;
	layout(offset = 12) float _Weight;
	layout(offset = 16) float _Exposure;
	layout(offset = 32) vec2 lightPosition;
	layout(offset = 48) vec4 color;

	// uint nrAtom;
	// Sphere sphere[3];
}
settings;

     const vec3 betaR = vec3(3.8e-6f, 13.5e-6f, 33.1e-6f);
     const vec3 betaM = vec3(21e-6f);

vec3 computeIncidentLight(const vec3 orig, const float atmosphereRadius, const float earthRadius, const vec3 dir,
						  float tmin, float tmax) {

	const vec3 sunDirection = vec3(0, 0, 1);

	float Hr = 7994; // Thickness of the atmosphere if density was uniform (Hr)
	float Hm = 1200; // Same as above but for Mie scattering (Hm)

	float t0, t1;
	const vec2 samples = ray_sphere_intersect_samples(vec3(0), atmosphereRadius, orig, dir);

	t0 = samples.x;
	t1 = samples.y;

	if (!(t0 <= -1 && t1 <= -1) || t1 < 0) {
		return vec3(0);
	}
	if (t0 > tmin && t0 > 0) {
		tmin = t0;
	}
	if (t1 < tmax) {
		tmax = t1;
	}

	uint numSamples = 16;
	uint numSamplesLight = 8;
	float segmentLength = (tmax - tmin) / numSamples;
	float tCurrent = tmin;
	vec3 sumR = vec3(0);
	vec3 sumM = vec3(0); // mie and rayleigh contribution
	float opticalDepthR = 0, opticalDepthM = 0;
	float mu = dot(dir, sunDirection); // mu in the paper which is the cosine of the angle between the sun direction and
									   // the ray direction
	float phaseR = 3.f / (16.f * PI) * (1 + mu * mu);
	float g = 0.76f;
	float phaseM =
		3.f / (8.f * PI) * ((1.f - g * g) * (1.f + mu * mu)) / ((2.f + g * g) * pow(1.f + g * g - 2.f * g * mu, 1.5f));

	for (uint i = 0; i < numSamples; ++i) {
		vec3 samplePosition = orig + (tCurrent + segmentLength * 0.5f) * dir;
		float height = samplePosition.length() - earthRadius;
		// compute optical depth for light
		float hr = exp(-height / Hr) * segmentLength;
		float hm = exp(-height / Hm) * segmentLength;
		opticalDepthR += hr;
		opticalDepthM += hm;
		// light optical depth
		float t0Light, t1Light;
		//TODO: Fix //raySphereIntersect(samplePosition, sunDirection, atmosphereRadius, t0Light, t1Light);
		float segmentLengthLight = t1Light / numSamplesLight, tCurrentLight = 0;
		float opticalDepthLightR = 0, opticalDepthLightM = 0;
		uint j;
		for (j = 0; j < numSamplesLight; ++j) {
			vec3 samplePositionLight = samplePosition + (tCurrentLight + segmentLengthLight * 0.5f) * sunDirection;
			float heightLight = samplePositionLight.length() - earthRadius;
			if (heightLight < 0) {
				break;
			}
			opticalDepthLightR += exp(-heightLight / Hr) * segmentLengthLight;
			opticalDepthLightM += exp(-heightLight / Hm) * segmentLengthLight;
			tCurrentLight += segmentLengthLight;
		}
		if (j == numSamplesLight) {
			vec3 tau =
				betaR * (opticalDepthR + opticalDepthLightR) + betaM * 1.1f * (opticalDepthM + opticalDepthLightM);
			vec3 attenuation = vec3(exp(-tau.x), exp(-tau.y), exp(-tau.z));
			sumR += attenuation * hr;
			sumM += attenuation * hm;
		}
		tCurrent += segmentLength;
	}

	// We use a magic number here for the intensity of the sun (20). We will make it more
	// scientific in a future revision of this lesson/code
	return (sumR * betaR * phaseR + sumM * betaM * phaseM) * 20;
}

void main() {

	vec3 position = vec3(0);
	float step_length = 0;
	for (uint j = 0; j < settings.numSamples; j++) {

		const vec3 ray = vec3(0);

		/*	here we calculate the sampling point position in view space.	*/
//		const vec3 samplePos = viewPos + ray * step_length;

		// const float geometryDepth = calcViewPosition(offset.xy).z;
	}
}
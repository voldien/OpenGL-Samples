#version 460 core
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(set = 0, binding = 0) uniform sampler2D SourceEnvTexture;

layout(push_constant) uniform Settings {
	layout(offset = 0) float sampleDelta;
	layout(offset = 4) float maxValue;
	layout(offset = 8) float roughness;
}
settings;

#include "common.glsl"
#include "pbr.glsl"

vec3 prefilter(const in vec3 N, const in float roughness) {

	// make the simplifying assumption that V equals R equals the normal
	vec3 R = N;
	vec3 V = R;

	const uint SAMPLE_COUNT = 1024u;
	highp vec3 prefilteredColor = vec3(0.0);
	highp float totalWeight = 0.0;

	/*	*/
	for (uint i = 0u; i < SAMPLE_COUNT; ++i) {

		// generates a sample vector that's biased towards the preferred alignment direction (importance sampling).
		const vec2 Xi = Hammersley(i, SAMPLE_COUNT);
		const vec3 H = ImportanceSampleGGX(Xi, N, roughness);
		const vec3 L = normalize(2.0 * dot(V, H) * H - V) * 0.9999; // TODO: fix in till equi recntugular look up.

		const float NdotL = max(dot(N, L), 0.0);
		if (NdotL > 0.0) {

			// sample from the environment's mip level based on roughness/pdf
			const float D = DistributionGGX(N, H, roughness);
			const float NdotH = max(dot(N, H), 0.0);
			const float HdotV = max(dot(H, V), 0.0);
			const float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

			float resolution = 2048.0; // resolution of source cubemap (per face)
			float saTexel = 4.0 * PI / (6.0 * resolution * resolution);
			float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

			float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

			const vec2 panoramic_coordinate = inverse_equirectangular(L);

			prefilteredColor += textureLod(SourceEnvTexture, panoramic_coordinate, mipLevel).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	prefilteredColor = prefilteredColor / totalWeight;

	return prefilteredColor;
}

void main() {

	const vec3 direction = normalize(vVertex);

	const vec3 prefilter_color = prefilter(direction, settings.roughness);

	fragColor = vec4(prefilter_color, 1);
}

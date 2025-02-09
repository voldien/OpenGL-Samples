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

#include "postprocessing_base.glsl"

layout(push_constant) uniform UniformBufferBlock {
	layout(offset = 0) int numSamples;
	layout(offset = 4) float _Density;
	layout(offset = 8) float _Decay;
	layout(offset = 12) float _Weight;
	layout(offset = 16) float _Exposure;
	layout(offset = 32) vec2 lightPosition;
	layout(offset = 48) vec4 Color;
}
settings;

void main() {

	/*	*/
	const vec2 _LightScreenPos = settings.lightPosition;
	const vec2 deltascreenUVrd = (screenUV - _LightScreenPos);

	/*	*/
	const float inverseDensity = ((1.0 / float(settings.numSamples)) * settings._Density);

	const vec2 TexDiff = deltascreenUVrd * inverseDensity;

	const vec4 color = texture(ColorTexture, screenUV);

	vec4 colorResult = vec4(0);
	float illuminationDecay = 1.0;

	for (uint j = 0; j < settings.numSamples; j++) {

		const vec2 OffsetUV = screenUV - TexDiff * j;

		/*	Determine if occluded by geometry.	*/
		float cameraDepth = texture(DepthTexture, OffsetUV).r;
		float mask = (cameraDepth < 1 ? 0.0 : 1.0);

		vec4 sampleColor = texture(ColorTexture, OffsetUV) * mask;
		/*	*/
		sampleColor *= illuminationDecay * settings._Weight;
		/*	*/
		colorResult += sampleColor;
		/*	*/
		illuminationDecay *= settings._Decay;
	}

	/*	*/
	colorResult /= float(settings.numSamples);

	colorResult *= settings._Exposure;

	/*	*/
	vec4 result = color + vec4(colorResult.rgb, 0.0);

	fragColor = result;
}

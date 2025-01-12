#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 uv;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 1) uniform sampler2D depthTexture;

#include "postprocessing_base.glsl"

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	uint numSamples;
	float _Density;
	float _Decay;
	float _Weight;
	float _Exposure;
}
ubo;

void main() {

	/*	*/
	const vec2 _LightScreenPos = vec2(0);
	vec2 deltauvrd = (uv - _LightScreenPos);

	/*	*/
	float inverseDensity = ((1.0 / float(ubo.numSamples)) * ubo._Density * ubo._Decay);

	vec2 TexDiff = deltauvrd * inverseDensity;

	vec4 color = texture(ColorTexture, uv);
	vec4 colorResult = vec4(0);

	float illuminationDecay = 1.0;

	for (int j = 0; j < ubo.numSamples; j++) {

		/*	Determine if occluded by geometry.	*/
		float cameraDepth = texture(depthTexture, uv).r;
		float mask = (cameraDepth < 1 ? 0.0 : 1.0);

		vec4 sampleColor = texture(ColorTexture, uv - TexDiff * j) * mask;

		/*	*/
		sampleColor *= illuminationDecay * ubo._Weight;
		/*	*/
		colorResult += sampleColor;
		/*	*/
		illuminationDecay *= ubo._Decay;
	}

	/*	*/
	colorResult /= float(ubo.numSamples);

	colorResult *= ubo._Exposure;

	/*	*/
	vec4 result = color + vec4(colorResult.rgb, 0.0);

	const float _Blend = 1;
	fragColor = mix(color, result, _Blend);
}

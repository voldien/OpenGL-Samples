#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 1) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 screenUV;

layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 6) uniform sampler2D EmissionTexture;

layout(push_constant) uniform Settings {
	layout(offset = 0) float variance;
	layout(offset = 4) float mean;
	layout(offset = 8) float radius;
	layout(offset = 12) int samples;
	layout(offset = 16) float threadshold;
}
settings;

#include "postprocessing_base.glsl"
layout(constant_id = 16) const int MAX_SAMPLES = 11;

vec4 blurHorizontal(const in float radius, const in float variance, const in int samples) {
	int y;

	const vec2 resolution = 1.0 / textureSize(ColorTexture, 0);

	const vec2 TexCoord = screenUV;

	const int start = clamp(-((samples - 1) / 2), -MAX_SAMPLES, -1);
	const int end = clamp(((samples - 1) / 2), 1, MAX_SAMPLES);

	vec4 color1 = vec4(0.0);
	float total = EPSILON;

	for (y = start; y <= end; y++) {

		const vec2 OffsetscreenUV = TexCoord + vec2(0, y) * radius * resolution;

		const float guas = getGuas2D(0, y, variance);

		color1 += texture(ColorTexture, OffsetscreenUV).rgba * vec4(guas.xxx, 1.0);
		total += guas;
	}

	return color1 / total;
}

vec4 blurVertical(const in float radius, const in float variance, const in int samples) {
	int x;

	const vec2 resolution = 1 / textureSize(ColorTexture, 0);

	const vec2 TexCoord = screenUV;

	const int start = clamp(-((samples - 1) / 2), -MAX_SAMPLES, -1);
	const int end = clamp(((samples - 1) / 2), 1, MAX_SAMPLES);

	vec4 color1 = vec4(0.0);
	float total = EPSILON;

	for (x = start; x <= end; x++) {
		const vec2 OffsetscreenUV = TexCoord + (vec2(x, 0) * radius * resolution);
		const float guas = getGuas2D(x, 0, variance);

		color1 += texture(ColorTexture, OffsetscreenUV).rgba * vec4(vec3(guas), 1.0);
		total += guas;
	}

	// Normalize color.
	return color1 / total;
}

void main() {
	/*	*/
	// check whether fragment output is higher than threshold, if so output as brightness color
	vec4 colorEmission = texture(ColorTexture, screenUV);
	float brightness = dot(colorEmission.rgb, vec3(0.2126, 0.7152, 0.0722));
	float enabled = brightness > 0.3 ? 1 : 0;

	const vec4 verticalBlur = blurVertical(settings.radius, settings.variance, settings.samples) * enabled;
	// memoryBarrierImage();
	fragColor = vec4(verticalBlur.rgb, 1.0);

	// memoryBarrierImage();

	const vec4 blurHorizontalBlur = blurHorizontal(settings.radius, settings.variance, settings.samples);

	// memoryBarrierImage();

	// fragColor = vec4(blurHorizontalBlur.rgb, 1.0);
}
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
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

#include "postprocessing_base.glsl"

layout(push_constant) uniform Settings {
	layout(offset = 0) BaseSettings base;
	layout(offset = 4) float redOffset;
	layout(offset = 8) float greenOffset;
	layout(offset = 12) float blueOffset;
	layout(offset = 16) vec2 direction_center;
}
settings;

void main() {

	const vec2 direction = screenUV - settings.direction_center;

	vec4 chromaticColor;
	chromaticColor.r = texture(ColorTexture, screenUV + (direction * vec2(settings.redOffset))).r;
	chromaticColor.g = texture(ColorTexture, screenUV + (direction * vec2(settings.greenOffset))).g;
	chromaticColor.ba = texture(ColorTexture, screenUV + (direction * vec2(settings.blueOffset))).ba;

	fragColor = mix(texture(ColorTexture, screenUV), chromaticColor, settings.base.blend);
}

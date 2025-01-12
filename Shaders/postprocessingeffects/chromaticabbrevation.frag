#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 uv;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;


layout(push_constant) uniform Settings {
	layout(offset = 0) float redOffset;
	layout(offset = 4) float greenOffset;
	layout(offset = 8) float blueOffset;
	layout(offset = 16) vec2 direction_center;
}
settings;

#include "postprocessing_base.glsl"

void main() {

	const vec2 direction = uv - settings.direction_center;

	fragColor.r = texture(ColorTexture, uv + (direction * vec2(settings.redOffset))).r;
	fragColor.g = texture(ColorTexture, uv + (direction * vec2(settings.greenOffset))).g;
	fragColor.ba = texture(ColorTexture, uv + (direction * vec2(settings.blueOffset))).ba;
}

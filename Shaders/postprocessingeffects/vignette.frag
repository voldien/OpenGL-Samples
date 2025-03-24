#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision highp float;
precision mediump int;

/*  */
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

#include "postprocessing_base.glsl"

layout(push_constant) uniform UniformSettingsBlock {
	layout(offset = 0) BaseSettings base;
	layout(offset = 4) float radius;
	layout(offset = 4) float extent;
}
settings;

void main() {

	const vec2 Offset = screenUV * (1.0 - screenUV.yx); // vec2(1.0)- uv.yx; -> 1.-u.yx; Thanks FabriceNeyret !

	float vig = Offset.x * Offset.y * 15.0; // multiply with sth for intensity
	vig = pow(vig, settings.extent);		// change pow for modifying the extend of the  vignette

	const vec4 colorTexture = texture(ColorTexture, screenUV);

	fragColor = mix(colorTexture, vig * colorTexture, settings.base.blend);
}
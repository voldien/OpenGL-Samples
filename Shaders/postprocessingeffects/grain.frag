#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_realtime_clock : enable

/*  */
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

layout(push_constant) uniform Settings {
	layout(offset = 0) float time;
	layout(offset = 4) float intensity;
	layout(offset = 8) float speed;
}
settings;

#include "postprocessing_base.glsl"

void main() {

	/*  */
	const float noise_value = simple_rand(screenUV * settings.time) * settings.intensity;

	fragColor = texture(ColorTexture, screenUV) + noise_value;
}

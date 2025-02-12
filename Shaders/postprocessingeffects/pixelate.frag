#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_realtime_clock : enable

/*  */
layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;

layout(push_constant) uniform Settings { layout(offset = 0) float size; }
settings;

#include "postprocessing_base.glsl"

void main() {

	const vec2 aspect_ratio = vec2(1.0, 1.0); // Maintain 1:1 aspect ratio for dots

	const vec2 pixelated_screenUV = pixelate_screenUV(screenUV, settings.size, aspect_ratio);
	fragColor = texture(ColorTexture, pixelated_screenUV);
}

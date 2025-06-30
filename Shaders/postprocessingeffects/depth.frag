#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_realtime_clock : enable

precision mediump float;
precision mediump int;

/*  */
layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D DepthTexture;

#include "postprocessing_base.glsl"

void main() {
	const float liner_depth = get_depth_linear(DepthTexture, screenUV, 0.15, 1000.0f);
	fragColor = vec4(liner_depth.xxx, 1.0);
}
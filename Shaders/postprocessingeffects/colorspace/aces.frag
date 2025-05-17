#version 460 core
#extension GL_ARB_derivative_control : enable
#extension GL_ARB_enhanced_layouts : enable
#extension GL_ARB_shader_image_load_store : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

layout(binding = 1) uniform sampler2D ColorTexture;


#include"common.glsl"

void main() {

	vec4 color = texture(ColorTexture, screenUV);
	fragColor = vec4(acesFilm(color.rgb), 1);
}
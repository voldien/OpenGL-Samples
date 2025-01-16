#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(early_fragment_tests) in;
layout(location = 0) out vec2 DepthResult;

layout(location = 0) in vec2 TextureCoord;

#include "scene.glsl"

void main() {

	DepthResult.x = gl_FragCoord.z;
	DepthResult.y = gl_FragCoord.z * gl_FragCoord.z;
}
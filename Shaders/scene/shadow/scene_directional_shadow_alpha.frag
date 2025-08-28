#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

#extension GL_ARB_conservative_depth : enable
#extension GL_EXT_conservative_depth : enable

precision mediump float;
precision mediump int;

layout(location = 0) in vec2 UV;

#if defined(GL_EXT_conservative_depth) || defined(GL_ARB_conservative_depth)
layout(depth_less) out float gl_FragDepth;
#endif

#include "scene.glsl"

void main() {

	const float alpha = texture(DiffuseTexture, UV).a * texture(AlphaMaskedTexture, UV).r;

	const material mat = getMaterial(0); // fAssigns.x

	const float clip = mat.clip_.x;
	if (alpha < clip) {
		discard;
	} else {
		gl_FragDepth = gl_FragCoord.z;
	}
}
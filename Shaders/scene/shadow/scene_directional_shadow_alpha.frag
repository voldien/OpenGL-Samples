#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

/*	*/
layout(location = 0) in vec2 UV;
/*	*/
layout(location = 8) flat in ivec2 fAssigns;

#include "scene.glsl"

void main() {

	/*	*/
	const float alpha = texture(DiffuseTexture, UV).a * texture(AlphaMaskedTexture, UV).r;

	/*	*/
	const material mat = getMaterial(fAssigns.x); //

	const float clip = mat.clip_.x * 0.5;
	if (alpha < clip) {
		discard;
	} else {
		gl_FragDepth = gl_FragCoord.z;
	}
}
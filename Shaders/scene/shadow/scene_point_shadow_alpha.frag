#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec4 FragVertex;
layout(location = 1) in flat int FIndex;
layout(location = 2) in vec2 TextureCoord;

#include "scene.glsl"

void main() {

	const PointLight pointLight = getPointLight(FIndex);

	/*	*/
	float lightDistance = length(pointLight.position - FragVertex.xyz);

	/*	map to [0;1].	*/
	lightDistance = lightDistance / pointLight.range;

	const float alpha = texture(DiffuseTexture, TextureCoord).a * texture(AlphaMaskedTexture, TextureCoord).r;

	const material mat = getMaterial(0); // fAssigns.x
	const float clip = mat.clip_.x;

	if (alpha < clip) {
		discard;
	} else {
		gl_FragDepth = lightDistance;
	}
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 TextureCoord;

layout(location = 4) in int index;
/*	*/
layout(location = 8) in ivec2 vAssigns;

layout(location = 0) out flat int GIndex; // invariant
layout(location = 1) out vec2 OutTextureCoord;

#include "scene.glsl"

void main() {

	const mat4 model = getModel(vAssigns.y);

	gl_Position = model * vec4(vertex, 1.0);
	GIndex = index;
	OutTextureCoord = TextureCoord;
}
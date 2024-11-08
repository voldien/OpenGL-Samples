#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 FUV;
layout(binding = 1) uniform sampler2D DiffuseTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;

	vec4 tintColor;

	/*  */
	vec4 cameraPosition;
	vec2 scale;
	vec4 offset;
}
ubo;

void main() {

	fragColor = texture(DiffuseTexture, FUV);
	if (fragColor.a < 0.5) {
		discard;
	}
}
#version 460
#extension GL_ARB_separate_shader_objects : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform samplerCube TextureCubeMap;

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 modelViewProjection;
	vec4 tintColor;
	/*	*/
	float exposure;
	float gamma;
}
ubo;

layout(early_fragment_tests) in;

void main() {

	fragColor = texture(TextureCubeMap, vVertex) * ubo.tintColor;

	// fragColor = vec4(1.0) - exp(-fragColor * ubo.exposure);
	// const float gamma = ubo.gamma;
	// fragColor = pow(fragColor, vec4(1.0 / gamma));
}
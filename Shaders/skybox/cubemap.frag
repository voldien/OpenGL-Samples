#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform samplerCube textureCubeMap;

layout(binding = 1) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	float exposure;
	float gamma;
}
ubo;

void main() {

	fragColor = textureLod(textureCubeMap, vVertex, 0) * ubo.tintColor;
	fragColor = vec4(1.0) - exp(-fragColor * ubo.exposure);

	const float gamma = 2.2;
	fragColor = pow(fragColor, vec4(1.0 / gamma));
}
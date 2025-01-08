#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vVertex;

layout(binding = 0) uniform sampler2D PanoramaTexture;

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	float exposure;
	float gamma;
}
ubo;

#include "common.glsl"

void main() {

	const vec2 uv = inverse_equirectangular(normalize(vVertex));

	fragColor = textureLod(PanoramaTexture, uv, 0) * ubo.tintColor;

	// fragColor = vec4(1.0) - exp(-fragColor * ubo.exposure);
	// const float gamma = ubo.gamma;
	// fragColor = pow(fragColor, vec4(1.0 / gamma));

	fragColor = fragColor * ubo.tintColor;
}
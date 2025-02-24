#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 1) uniform sampler2D DepthTexture;

#include "fog_frag.glsl"
#include "postprocessing_base.glsl"

layout(set = 0, binding = 1, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 viewRotation;
	Camera camera;
	FogSettings fogSettings;
}
ubo;

void main() {

	const float depth = texture(DepthTexture, screenUV).r;

	const float fogFactor = min(getFogFactor(ubo.fogSettings, depth), 1);

	fragColor.rgb = mix(texture(ColorTexture, screenUV).rgb, ubo.fogSettings.fogColor.rgb, fogFactor);
	fragColor.a = 1;
}
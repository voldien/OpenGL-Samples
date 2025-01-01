#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 uv;

/*  */
layout(set = 0, binding = 0) uniform sampler2D texture0;
layout(set = 0, binding = 1) uniform sampler2D DepthTexture;
layout(set = 0, binding = 2) uniform sampler2D IrradianceTexture;

#include "fog_frag.glsl"
#include "postprocessing_base.glsl"

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	FogSettings fogSettings;
	Camera camera;
}
ubo;

void main() {

	const vec3 direction = vec3(1);

		/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(direction));
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	const float depth = texture(DepthTexture, gl_FragCoord.xy).r;

	const float fogFactor = getFogFactor(ubo.fogSettings, depth);

	fragColor.rgb = mix(texture(texture0, uv).rgb, irradiance_color.rgb, fogFactor);
	fragColor.a = 1;
}

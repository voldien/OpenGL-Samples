#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 1) uniform sampler2D DepthTexture;
layout(set = 0, binding = 2) uniform sampler2D IrradianceTexture;

#include "fog_frag.glsl"
#include "postprocessing_base.glsl"

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 viewRotation;
	Camera camera;
	FogSettings fogSettings;
}
ubo;

void main() {

	const vec3 direction = normalize(mat3(ubo.viewRotation) * vec3(2 * screenUV - 1, 1)); //(ubo.proj * ).xyz;

	vec3 cr = normalize(cross(direction, vec3(0, -1, 0)));

	const float aat = 1;// clamp(pow(abs(1 - dot(direction, vec3(0, 1, 0))), 1.05), 0, 1);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(direction);
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	const float depth = texture(DepthTexture, screenUV).r;

	const float fogFactor = getFogFactor(ubo.fogSettings, depth);

	fragColor.rgb = mix(texture(ColorTexture, screenUV).rgb, irradiance_color.rgb * ubo.fogSettings.fogColor.rgb, clamp(fogFactor * aat, 0, 1));
	// fragColor.rgb = vec3(aat); // direction;
	fragColor.a = 1;
}

#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;

#include "common.glsl"
#include "phongblinn.glsl"

layout(constant_id = 10) const int MaxWaves = 128;

struct Wave {
	float wavelength;	/*	*/
	float amplitude;	/*	*/
	float speed;		/*	*/
	float rolling;		/*	*/
	vec2 direction;		/*	*/
	vec2 creast_offset; /*	*/
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	DirectionalLight directional;
	Camera camera;

	Wave waves[MaxWaves];
	int nrWaves;
	float time;
	float stepness;
	float rolling;

	/*	Material	*/

	vec4 oceanColor;
	vec4 specularColor;
	vec4 ambientColor;
	float shininess;
	float fresnelPower;
}
ubo;

layout(binding = 0) uniform sampler2D ReflectionTexture;

layout(binding = 10) uniform sampler2D IrradianceTexture;

void main() {

	// Create new normal per pixel based on the normal map.
	const vec3 Mnormal = normalize(normal);

	/*	*/
	const vec3 viewDir = normalize(ubo.camera.position.xyz - vertex);

	const vec4 lightColor =
		computePhongDirectional(ubo.directional, Mnormal, viewDir, ubo.shininess, ubo.specularColor.rgb);

	/*	*/
	const vec3 reflection = normalize(reflect(-viewDir, Mnormal));
	const vec2 reflection_uv = inverse_equirectangular(reflection);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(Mnormal));
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	/*	*/
	const vec3 fresnel = FresnelSchlick(vec3(0.02) * ubo.fresnelPower, viewDir, normal);

	const vec4 gradientWaterDepth = vec4(0);

	const vec4 color = mix(ubo.oceanColor, texture(ReflectionTexture, reflection_uv), vec4(fresnel.rgb, 1));
	fragColor = color * (ubo.ambientColor * irradiance_color + lightColor);
	fragColor.a = 1;
}
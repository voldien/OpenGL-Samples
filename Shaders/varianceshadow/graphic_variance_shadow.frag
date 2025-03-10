#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

// layout(early_fragment_tests) in;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 lightSpace;

layout(binding = 9) uniform sampler2DShadow ShadowTexture;

#include "common.glsl"
#include "scene.glsl"
#include "phongblinn.glsl"
#include"pbr.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 lightSpaceMatrix;

	/*	Light source.	*/
	DirectionalLight directional;
	Camera camera;

	/*	*/
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;

	/*	*/
	float bias;
	float shadowStrength;
	float radius;
}
ubo;

float ShadowCalculation(const in vec4 fragPosLightSpace) {

	// perform perspective divide
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

	if (fragPosLightSpace.w > 1.0) {
		return 1;
	}

	// transform from NDC to Screen Space [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	const float bias =
		clamp(0.05 * (1.0 - dot(normalize(normal), normalize(ubo.directional.direction).xyz)), 0, ubo.bias);
	projCoords.z *= (1 - bias);

	const float shadow = textureProj(ShadowTexture, projCoords, 0).r;

	return (1.0 - shadow);
}

void main() {

	const material mat = getMaterial();
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	const vec3 NewNormal = getNormalFromMap(NormalTexture, UV, vertex, normal, mat.clip_.y);

	const vec3 viewDir = normalize(ubo.camera.position.xyz - vertex);

	const float shadow = max(ubo.shadowStrength - ShadowCalculation(lightSpace), 0);
	/*	*/
	vec4 lightColor =
		computeBlinnDirectional(ubo.directional, NewNormal, viewDir, ubo.specularColor.a, mat.specular_roughness.rgb);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(NewNormal));
	const vec4 irradiance_color = vec4(texture(IrradianceTexture, irradiance_uv).rgb, 1);

	const vec4 color = texture(DiffuseTexture, UV) * mat.diffuseColor;
	const vec4 lighting = (glob_settings.ambientColor * mat.ambientColor * irradiance_color + lightColor * shadow);

	fragColor = vec4(lighting.rgb, 1) * color;
	fragColor.a *= texture(AlphaMaskedTexture, UV).r;
	fragColor *= mat.transparency.rgba;
	fragColor.rgb += mat.emission.rgb * texture(EmissionTexture, UV).rgb;
	if (fragColor.a < 0.8) {
		discard;
	}
}

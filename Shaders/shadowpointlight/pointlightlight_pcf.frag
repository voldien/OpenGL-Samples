#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;

#include "common.glsl"
#include "scene.glsl"

#include "phongblinn.glsl"

layout(binding = 16) uniform samplerCube ShadowTexture[4];

struct point_light {
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;

	float bias;
	float shadowStrength;
	float padding0;
	float padding1;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;

	point_light point_light[4];
	vec4 PCFFilters[20];
	float diskRadius;
	int samples;
}
ubo;

float ShadowCalculation(const in vec3 fragPosLightSpace, const in samplerCube ShadowTexture, const in uint index) {

	const vec3 frag2Light = (fragPosLightSpace - ubo.point_light[index].position);

	// float bias = ubo.point_light[0].bias;
	float bias = max(0.05 * (1.0 - dot(normalize(normal), -normalize(frag2Light).xyz)), ubo.point_light[index].bias);

	float currentDepth = length(frag2Light);

	float shadowFactor = 0;
	const ivec2 gMapSize = textureSize(ShadowTexture, 0);

	/*	*/
	if (currentDepth > ubo.point_light[index].range) {
		return 1.0;
	}

	const float far_plane = 1000; // FIXME:
	const float viewDistance = length(ubo.cameraPosition.xyz - fragPosLightSpace);
	const float diskRadius = (1.0 + (viewDistance / far_plane)) / ubo.diskRadius;
	const int samples = 20;

	[[unroll]] for (uint i = 0; i < samples; i++) {
		/*	*/
		float closestDepth = texture(ShadowTexture, frag2Light + ubo.PCFFilters[i].xyz * diskRadius).r;
		closestDepth *= far_plane; // undo mapping [0;1]

		shadowFactor += currentDepth - bias > closestDepth ? 0.0 : 1.0;
	}

	return shadowFactor / float(samples);
}

void main() {

	const material mat = getMaterial();
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	vec4 pointLightColors = vec4(0);

	[[unroll]] for (uint i = 0; i < 4; i++) {
		/*	*/
		vec3 diffVertex = (ubo.point_light[i].position - vertex);

		/*	*/
		float dist = length(diffVertex);

		/*	*/
		float attenuation =
			1.0 / (ubo.point_light[i].constant_attenuation + ubo.point_light[i].linear_attenuation * dist +
				   ubo.point_light[i].qudratic_attenuation * (dist * dist));

		float contribution = max(dot(normal, normalize(diffVertex)), 0.0);

		float shadow = ShadowCalculation(vertex, ShadowTexture[i], i);

		/*	*/
		pointLightColors += (attenuation * ubo.point_light[i].color * contribution * ubo.point_light[i].range *
							 ubo.point_light[i].intensity) *
							shadow;
	}
	pointLightColors.a = 1;
	fragColor = texture(DiffuseTexture, UV) * mat.diffuseColor * (glob_settings.ambientColor * mat.ambientColor + pointLightColors);

	fragColor.a *= texture(AlphaMaskedTexture, UV).r;
	fragColor *= mat.transparency.rgba;
	fragColor.rgb += mat.emission.rgb * texture(EmissionTexture, UV).rgb;
	if (fragColor.a < 0.8) {
		discard;
	}
}
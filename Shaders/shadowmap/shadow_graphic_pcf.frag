#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

// 7layout(early_fragment_tests) in;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 lightSpace;
layout(location = 8) flat in ivec2 fAssigns;

layout(binding = 9) uniform sampler2DShadow ShadowTexture;

layout(constant_id = 16) const int PCF_SAMPLES = 7;

#include "common.glsl"
#include "pbr.glsl"
#include "phongblinn.glsl"
#include "scene.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 lightSpaceMatrix;

	/*	Light source.	*/
	DirectionalLight directional;
	float bias;
	float shadowStrength;
	float radius;
}
ubo;

float ShadowCalculationPCF(const in vec4 fragPosLightSpace) {

	// perform perspective divide
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

	if (fragPosLightSpace.w > 1.0 || projCoords.z > 1.0) {
		return 0;
	}

	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	const float bias =
		clamp(0.005 * (1.0 - dot(normalize(normal), normalize(-ubo.directional.direction).xyz)), 0.0005, ubo.bias);
	projCoords.z *= (1 - bias);


	float shadowFactor = 0;
	const ivec2 gMapSize = textureSize(ShadowTexture, 0);

	const float xOffset = 1.0 / gMapSize.x * ubo.radius;
	const float yOffset = 1.0 / gMapSize.y * ubo.radius;

	const float nrSamples = float(PCF_SAMPLES) * float(PCF_SAMPLES);

	/*	*/
	[[unroll]] for (int y = -PCF_SAMPLES / 2; y <= PCF_SAMPLES / 2; y++) {
		[[unroll]] for (int x = -PCF_SAMPLES / 2; x <= PCF_SAMPLES / 2; x++) {

			const vec2 Offsets = vec2(x * xOffset, y * yOffset);

			const vec3 UVC = vec3(projCoords.xy + Offsets, projCoords.z + EPSILON);
			shadowFactor += texture(ShadowTexture, UVC);
		}
	}

	return (1.0 - (shadowFactor / nrSamples));
}

void main() {

	const material mat = getMaterial(fAssigns.x);
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	vec3 viewDir = normalize(getCamera().position.xyz - vertex);

	const vec3 NewNormal = getNormalFromMap(NormalTexture, UV, vertex, normal, mat.clip_.y);

	const float shadow = max(1 - ShadowCalculationPCF(lightSpace) * ubo.shadowStrength, 0);

	/*	*/
	const vec4 SpecularColor = vec4(mat.specular_roughness.rgb, 1) * texture(RoughnessTexture, UV);
	const vec4 lightColor = computeBlinnDirectional(ubo.directional, NewNormal, viewDir, mat.specular_roughness.a,
													SpecularColor.rgb);

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(NewNormal));
	const vec4 irradiance_color = texture(IrradianceTexture, irradiance_uv).rgba;

	const vec4 color = texture(DiffuseTexture, UV) * mat.diffuseColor;
	const vec4 lighting = (glob_settings.ambientColor * mat.ambientColor * irradiance_color + lightColor * shadow);

	fragColor = vec4(lighting.rgb, 1) * color;
	fragColor.a *= texture(AlphaMaskedTexture, UV).r;
	fragColor *= mat.transparency.rgba;
	fragColor.rgb += mat.emission.rgb * texture(EmissionTexture, UV).rgb;
	if (fragColor.a < mat.clip_.x) {
		discard;
	}
}
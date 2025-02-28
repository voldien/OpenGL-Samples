#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 8) flat in ivec2 fAssigns;

#include "pbr.glsl"
#include "pbr_common.glsl"

void main() {

	const material mat = getMaterial(fAssigns.x);
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	/*	material properties	*/
	vec3 albedo = texture(DiffuseTexture, TexCoords).rgb;
	float metallic = texture(MetalicTexture, TexCoords).r - 1;
	float roughness = texture(RoughnessTexture, TexCoords).r;
	float ao = texture(AOTexture, TexCoords).r;

	// input lighting data
	vec3 N = getNormalFromMap(NormalTexture, TexCoords, WorldPos, Normal);
	vec3 V = normalize(getCamera().position.xyz - WorldPos);
	vec3 R = reflect(-V, N);

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// TODO: add light
	vec3 Lo = vec3(0.0);

	// ambient lighting (we now use IBL as the ambient term)
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	/*	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(N));
	const vec4 irradiance_color = vec4(texture(IrradianceTexture, irradiance_uv).rgb, 1);
	vec3 diffuse = (glob_settings.ambientColor.rgb * irradiance_color.rgb * mat.ambientColor.rgb) * albedo * mat.diffuseColor.rgb;

	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to
	// get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = vec3(0); // prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	vec3 color = ambient + Lo;

	fragColor = vec4(color, 1.0);
	fragColor.a *= texture(AlphaMaskedTexture, TexCoords).r;
	fragColor *= mat.transparency.rgba;
	fragColor.rgb += mat.emission.rgb * texture(EmissionTexture, TexCoords).rgb;
	if (fragColor.a < 0.8) {
		discard;
	}
}

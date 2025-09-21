#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision mediump float;
precision mediump int;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 8) flat in ivec2 fAssigns;

#include "common_frag.glsl"
#include "pbr.glsl"
#include "pbr_common.glsl"

float computeLODFromRoughness(const in float perceptualRoughness) { return perceptualRoughness; }

void main() {

	/*	*/
	const material mat = getMaterial(fAssigns.x);
	const global_rendering_settings glob_settings = getRenderingSettings();

	/*	Material properties.	*/
	const vec4 albedo = texture(DiffuseTexture, TexCoords);
	const float metallic = texture(MetalicTexture, TexCoords).b * mat.clip_.z;
	const float roughness = clamp(texture(RoughnessTexture, TexCoords).g * mat.specular_roughness.a, 0.0001, 1);
	const float ao = texture(AOTexture, TexCoords).r;
	const vec3 emissive = mat.emission.rgb * texture(EmissionTexture, TexCoords).rgb;

	/*	input lighting data	*/
	const vec3 SurfaceNormal = getTBN(Normal, Tangent, NormalTexture, mat.clip_.y, TexCoords);
	// getNormalFromMap(NormalTexture, TexCoords, WorldPos, Normal, mat.clip_.y);
	vec3 ViewPixelDir = normalize(getCamera().position.xyz - WorldPos);
	vec3 ViewPixelReflectDir = normalize(reflect(-ViewPixelDir, SurfaceNormal));

	/*	Interpolated between non-metal to metal fresnel factor.	*/
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo.rgb, metallic);

	const float perceivedRoughness = roughness * roughness;

	/*	Directional Light.	*/
	vec3 DirectLight = vec3(0.0);
	for (uint light_index = 0; light_index < getDirectionalLightCount(); light_index++) {

		/*	Calculate light color.	*/
		const vec3 lightSource = computePBRDirectionLight(getDirectional(light_index), ViewPixelDir, SurfaceNormal,
														  roughness, metallic, F0, albedo.rgb);

		/*	Shadow.	*/
		const vec4 LightSpaceVertex = getDirectional(light_index).lightShadow.lightSpaceMatrix * vec4(WorldPos, 1);

		float shadow = 1;
		if (ShadowMapMode == SHADOW_MODE_HARD) {
			shadow = DirectionalShadowCalculation(getDirectional(light_index), DirectionalShadowTexture[light_index],
												  SurfaceNormal, LightSpaceVertex);
		} else if (ShadowMapMode == SHADOW_MODE_SOFT) {
			shadow = ShadowCalculationPCF(getDirectional(light_index), DirectionalShadowTexture[light_index],
										  SurfaceNormal, LightSpaceVertex);
		}

		DirectLight += lightSource * shadow;
	}

	/*	Point Lights.	*/
	for (uint i = 0; i < getPointLightCount(); i++) {
		DirectLight += computePBRPoint(getPointLight(i), WorldPos, ViewPixelDir, SurfaceNormal, roughness, metallic, F0,
									   albedo.rgb);
	}

	/*	ambient lighting (we now use IBL as the ambient term)	*/
	vec3 kSpecular_F = fresnelSchlick(max(dot(SurfaceNormal, ViewPixelDir), 0.0), F0); // roughness

	/*	*/
	vec3 kDiffuse = 1.0 - kSpecular_F;
	kDiffuse *= 1.0 - metallic;

	const vec4 diffuseColor = albedo * mat.diffuseColor;

	vec3 indirectLight;
	if (UseImageBasedLightning) {

		/*	Diffuse.	*/
		const vec4 diffuse_irradiance_color = vec4(texture(IrradianceTexture, SurfaceNormal).rgb, 1);
		const vec3 diffuse_irradiance_color_contr =
			glob_settings.ambientColor.rgb * diffuse_irradiance_color.rgb * mat.ambientColor.rgb;

		const vec3 diffuse = diffuse_irradiance_color_contr * diffuseColor.rgb;

		// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation
		// to get the IBL specular part.
		const float MAX_REFLECTION_LOD = 5.0;
		const vec2 irradiance_specular_uv = inverse_equirectangular(normalize(ViewPixelReflectDir));

		const vec3 prefilteredColor =
			textureLod(prefilterMap, irradiance_specular_uv, roughness * MAX_REFLECTION_LOD).rgb;
		const vec2 brdf_lookup_uv = vec2(max(dot(SurfaceNormal, ViewPixelDir), 0.0), roughness);
		const vec2 brdf = texture(BRDFLUT, brdf_lookup_uv).rg;

		vec3 specular = prefilteredColor * (kSpecular_F * brdf.x + brdf.y);
		specular *= mat.specular_roughness.rgb;
		specular *= glob_settings.specularColor.rgb;

		indirectLight = (kDiffuse * diffuse + specular) * ao;
	} else {
		indirectLight = vec3(0);
	}

	const vec3 color = emissive + indirectLight + DirectLight;

	fragColor = vec4(color, 1.0);

	/*	Alpha.	*/
	const float alpha = diffuseColor.a * texture(AlphaMaskedTexture, TexCoords).r * mat.transparency.a;
	fragColor.a = mix(alpha, 1, kSpecular_F.x);
	fragColor.rgb *= mat.transparency.rgb;

	if (UseClipping) {
		const float clipRange = mat.clip_.x;
		if (fragColor.a < clipRange) {
			discard;
		}
	}
}

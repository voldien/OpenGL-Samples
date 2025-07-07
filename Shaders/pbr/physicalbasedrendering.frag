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

	/*	*/
	const material mat = getMaterial(fAssigns.x);
	const global_rendering_settings glob_settings = constantCommon.constant.globalSettings;

	/*	Material properties.	*/
	const vec3 albedo = texture(DiffuseTexture, TexCoords).rgb;
	const float metallic = texture(MetalicTexture, TexCoords).r;
	const float roughness = clamp(texture(RoughnessTexture, TexCoords).r * mat.specular_roughness.a, 0, 1);
	const float ao = texture(AOTexture, TexCoords).r;
	const vec3 emissive = mat.emission.rgb * texture(EmissionTexture, TexCoords).rgb;

	/*	input lighting data	*/
	vec3 SurfaceNormal = getNormalFromMap(NormalTexture, TexCoords, WorldPos, Normal, mat.clip_.y);
	vec3 ViewPixelDir = normalize(getCamera().position.xyz - WorldPos);
	vec3 ViewPixelReflectDir = reflect(-ViewPixelDir, SurfaceNormal);

	/*	Interpolated between non-metal to metal fresnel factor.	*/
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	/*	Directional Light.	*/
	vec3 Lo = vec3(0.0);
	for (int i = 0; i < LightUBO.light.directionalCount; i++) {
		Lo += computePBRDirectionLight(LightUBO.light.directional[i], ViewPixelDir, SurfaceNormal, roughness, metallic,
									   F0, albedo);
	}

	/*	Point Lights.	*/
	for (int i = 0; i < LightUBO.light.pointCount; i++) {
		Lo += computePBRPoint(LightUBO.light.point[i], WorldPos, ViewPixelDir, SurfaceNormal, roughness, metallic,
									   F0, albedo);
	}
	
	

	/*	ambient lighting (we now use IBL as the ambient term)	*/
	vec3 kSpecular_F = fresnelSchlickRoughness(max(dot(SurfaceNormal, ViewPixelDir), 0.0), F0, roughness);

	/*	*/
	vec3 kDiffuse = 1.0 - kSpecular_F;
	kDiffuse *= 1.0 - metallic;

	/*	Diffuse.	*/
	const vec2 irradiance_uv = inverse_equirectangular(normalize(SurfaceNormal));
	const vec4 diffuse_irradiance_color = vec4(texture(IrradianceTexture, irradiance_uv).rgb, 1);
	const vec3 diffuse = (glob_settings.ambientColor.rgb * diffuse_irradiance_color.rgb * mat.ambientColor.rgb) *
						 albedo * mat.diffuseColor.rgb;

	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to
	// get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	const vec3 prefilteredColor = textureLod(prefilterMap, ViewPixelReflectDir, roughness * MAX_REFLECTION_LOD).rgb;
	const vec2 brdf = texture(brdfLUT, vec2(max(dot(SurfaceNormal, ViewPixelDir), 0.0), roughness)).rg;

	vec3 specular = vec3(0); // prefilteredColor * (F * brdf.x + brdf.y);
	specular *= mat.specular_roughness.rgb;

	const vec3 ambient = (kDiffuse * diffuse + specular) * ao;

	const vec3 color = emissive + ambient + Lo;

	fragColor = vec4(color, 1.0);
	fragColor.a *= texture(AlphaMaskedTexture, TexCoords).r;
	fragColor *= mat.transparency.rgba;

	if (fragColor.a < mat.clip_.x) {
		discard;
	}
}

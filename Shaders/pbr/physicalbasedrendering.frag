#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 WorldPos;
layout(location = 1) in vec2 TexCoords;
layout(location = 2) in vec3 Normal;

#include "pbr_common.glsl"
#include "pbr.glsl"

// ----------------------------------------------------------------------------
void main() {
	/*	material properties	*/
	vec3 albedo = texture(DiffuseTexture, TexCoords).rgb;
	float metallic = texture(MetalicTexture, TexCoords).r;
	float roughness = texture(RoughnessTexture, TexCoords).r;
	float ao = texture(AOTexture, TexCoords).r;

	// input lighting data
	vec3 N = vec3(0); // getNormalFromMap(normalMap, );
	vec3 V = normalize(ubo.camera.position.xyz - WorldPos);
	vec3 R = reflect(-V, N);

	// calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
	// of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	// reflectance equation
	vec3 Lo = vec3(0.0);
	for (uint i = 0; i < 4; ++i) {

		// calculate per-light radiance
		vec3 L = vec3(0);// normalize(ubo.lightsettings.point_light[i].position - WorldPos);
		vec3 H = normalize(V + L);
		float distance = 1 ;// length(ubo.lightsettings.point_light[i].position - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = vec3(0); //ubo.lightsettings.point_light[i].color.rgb * attenuation;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		const vec3 F = vec3(0);// fresnelSchlick(max(dot(H, V), 0.0), F0);

		const vec3 numerator = NDF * G * F;
		const float denominator =
			4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
		vec3 specular = numerator / denominator;

		// kS is equal to Fresnel
		vec3 kS = F;
		// for energy conservation, the diffuse and specular light can't
		// be above 1.0 (unless the surface emits light); to preserve this
		// relationship the diffuse component (kD) should equal 1.0 - kS.
		vec3 kD = vec3(1.0) - kS;
		// multiply kD by the inverse metalness such that only non-metals
		// have diffuse lighting, or a linear blend if partly metal (pure metals
		// have no diffuse light).
		kD *= 1.0 - metallic;

		// scale light by NdotL
		float NdotL = max(dot(N, L), 0.0);

		// add to outgoing radiance Lo
		Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the
																// Fresnel (kS) so we won't multiply by kS again
	}

	// ambient lighting (we now use IBL as the ambient term)
	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic;

	vec3 irradiance = texture(IrradianceTexture, N.xy).rgb;
	vec3 diffuse = irradiance * albedo;

	// sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to
	// get the IBL specular part.
	const float MAX_REFLECTION_LOD = 4.0;
	vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
	vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	vec3 color = vec3(0); // ambient + Lo;

	fragColor = vec4(color, 1.0);
}
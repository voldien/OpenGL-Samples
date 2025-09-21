#ifndef _COMMON_LIGHT_SHADOW_H_
#define _COMMON_LIGHT_SHADOW_H_ 1
#include "light.glsl"

float calculteBias(const in vec3 SurfaceNormal, const in vec3 lightDirection, const in float bias) { return 0; }

/*	1 => No Shadow. 0 => Shadow.	*/
float DirectionalShadowCalculation(const in DirectionalLight directionLight, const in sampler2DShadow ShadowTexture,
								   const in vec3 surfaceNormal, const in vec4 VertexLightSpace) {

	/*	No Shadow Required.*/
	if (directionLight.lightShadow.shadow.x <= 0) {
		return 1;
	}

	// perform perspective divide
	vec4 projCoords = VertexLightSpace.xyzw / VertexLightSpace.w;

	if (VertexLightSpace.w > 1.0 || projCoords.z > 1.0) {
		return 1;
	}

	/*	transform from NDC to Screen Space [0,1] range	*/
	projCoords.xyz = projCoords.xyz * 0.5 + 0.5;

	const float light_shadow_bias = directionLight.lightShadow.shadow[1];
	const float bias = clamp(
		light_shadow_bias * (1.0 - dot(normalize(surfaceNormal), normalize(-directionLight.direction).xyz)), 0.0000, 1);
	projCoords.z *= (1 - bias);

	/*	shadow == 1 => Shadow, shadow == 0 => light.	*/
	const float shadow = 1 - textureProj(ShadowTexture, projCoords).r;

	const float shadowStrength = directionLight.lightShadow.shadow[0];
	return 1 - shadow * shadowStrength;
}

/*	1 => No Shadow. 0 => Shadow.	*/
float ShadowCalculationPCF(const DirectionalLight directionLight, const in sampler2DShadow ShadowTexture,
						   const in vec3 surfaceNormal, const in vec4 VertexLightSpace) {

	ShadowLight shadow = directionLight.lightShadow;
	const float pcf_radius = shadow.shadow[2];
	const int PCF_SAMPLES = 16; // TODO: add constant or something.

	// perform perspective divide
	vec4 projCoords = VertexLightSpace.xyzw / VertexLightSpace.w;

	if (VertexLightSpace.w > 1.0 || projCoords.z > 1.0) {
		return 1;
	}

	// transform NDC to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	const float light_shadow_bias = shadow.shadow[1];
	const float bias = clamp(
		light_shadow_bias * (1.0 - dot(normalize(surfaceNormal), normalize(-directionLight.direction).xyz)), 0.0000, 1);
	projCoords.z *= (1 - bias);

	float shadowFactor = 0;
	const ivec2 gMapSize = textureSize(ShadowTexture, 0);

	const float xOffset = (1.0 / gMapSize.x) * pcf_radius;
	const float yOffset = (1.0 / gMapSize.y) * pcf_radius;

	const float nrSamples = float(PCF_SAMPLES) * float(PCF_SAMPLES);

	/*	*/	//TODO: check equation
	[[unroll]] for (int y = -PCF_SAMPLES / 2; y <= PCF_SAMPLES / 2; y++) {
		[[unroll]] for (int x = -PCF_SAMPLES / 2; x <= PCF_SAMPLES / 2; x++) {

			const vec2 Offsets = vec2(x * xOffset, y * yOffset);

			const vec3 UVC = vec3(projCoords.xy + Offsets, projCoords.z + EPSILON);
			shadowFactor += 1 - texture(ShadowTexture, UVC);
		}
	}

	const float shadowContribution = (shadowFactor / nrSamples);

	const float shadowStrength = directionLight.lightShadow.shadow[0];
	return  clamp( 1 - (1 - (1 - shadowContribution * shadowStrength)), 0, 1);
}

float ShadowPointCalculation(const in PointLight pointLight, const in vec3 surfaceNormal, const in vec3 cameraPosition,
							 const in vec3 fragPosLightSpace, const in samplerCube ShadowTexture) {

	const vec3 frag2Light = (fragPosLightSpace - pointLight.position);

	float bias =
		max(0.05 * (1.0 - dot(normalize(surfaceNormal), -normalize(frag2Light).xyz)), pointLight.lightShadow.shadow.y);

	float currentDepth = length(frag2Light);

	float shadowFactor = 0;
	const ivec2 gMapSize = textureSize(ShadowTexture, 0);

	/*	Outside the shadow range. -> default to light.	*/
	if (currentDepth >= pointLight.range) {
		return 1.0;
	}

	const float far_plane = 1000; // TODO: make it the shadow cubemap far instead
	const float closestDepth = texture(ShadowTexture, frag2Light).r * far_plane;
	return currentDepth - bias > closestDepth ? 0.0 : 1.0;
}

float ShadowPointCalculationPCF(const in PointLight pointLight, const in vec3 surfaceNormal,
								const in vec3 cameraPosition, const in vec3 fragPosLightSpace,
								const in samplerCube ShadowTexture) {

	const vec3 frag2Light = (fragPosLightSpace - pointLight.position);

	float bias =
		max(0.05 * (1.0 - dot(normalize(surfaceNormal), -normalize(frag2Light).xyz)), pointLight.lightShadow.shadow.y);

	float currentDepth = length(frag2Light);

	float shadowFactor = 0;
	const ivec2 gMapSize = textureSize(ShadowTexture, 0);

	/*	Outside the shadow range. -> default to light.	*/
	if (currentDepth >= pointLight.range) {
		return 1.0;
	}

	return 1;

	// 	const float far_plane = 1000; // TODO: make it the shadow cubemap far instead
	// 	const float viewDistance = length(cameraPosition - fragPosLightSpace);
	// 	const float diskRadius = (1.0 + (viewDistance / far_plane)) / ubo.diskRadius;
	// 	const int samples = 20;

	// 	[[unroll]] for (uint i = 0; i < samples; i++) {
	// 		/*	*/
	// 		float closestDepth = texture(ShadowTexture, frag2Light + ubo.PCFFilters[i].xyz * diskRadius).r;
	// 		closestDepth *= far_plane; // undo mapping [0;1]

	// 		shadowFactor += currentDepth - bias > closestDepth ? 0.0 : 1.0;
	// 	}

	// 	/*	*/
	// 	return shadowFactor / float(samples);
}

#endif

#ifndef _COMMON_LIGHT_SHADOW_H_
#define _COMMON_LIGHT_SHADOW_H_ 1
#include"light.glsl"
// Shadow.

float ShadowCalculation(const in DirectionalLight directionLight, const in sampler2DShadow ShadowTexture, const in vec3 surfaceNormal, const in vec4 VertexLightSpace) {

		/*	No Shadow Required.*/
	if(directionLight.lightShadow.shadow.x <= 0){
		return 1;
	}

	// perform perspective divide
	vec4 projCoords = VertexLightSpace.xyzw / VertexLightSpace.w;

	if (VertexLightSpace.w > 1.0) {
		return 1;
	}

	/*	transform from NDC to Screen Space [0,1] range	*/ // ? z ??
	projCoords.xyz = projCoords.xyz * 0.5 + 0.5;

	const float light_shadow_bias = directionLight.lightShadow.shadow[1];
	const float bias =
		clamp(light_shadow_bias * (1.0 - dot(normalize(surfaceNormal), normalize(-directionLight.direction).xyz)), 0.0000, 1);
	projCoords.z *= (1 - bias);

	/*	*/
	const float shadow = textureProj(ShadowTexture, projCoords, 0).r;

	return (1.0 - shadow);
}


float ShadowCalculationPCF(const DirectionalLight directionLight, const in vec4 fragPosLightSpace) {

	// perform perspective divide
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

	if (fragPosLightSpace.w > 1.0 || projCoords.z > 1.0) {
		return 0;
	}

	// transform NDC to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	const float light_shadow_bias = directionLight.lightShadow.shadow[1];
	const float bias =
		clamp(light_shadow_bias * (1.0 - dot(normalize(normal), normalize(-directionLight.direction).xyz)), 0.0000, 1);
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

#endif

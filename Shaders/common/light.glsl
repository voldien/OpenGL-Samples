#ifndef _COMMON_LIGHT_H_
#define _COMMON_LIGHT_H_ 1

struct DirectionalLight {
	vec4 direction;
	vec4 lightColor;
};

struct PointLight {
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

float computeLightContributionFactor(const in vec3 direction, const in vec3 normalInput) {
	/*	*/
	return max(0.0, dot(-direction, normalInput));
}

vec4 computePoint(const in PointLight light, const in vec3 normal, const in vec3 vertex, const in float shininess,
				  const in vec3 specularColor) {

	/*	*/
	vec3 diffVertex = (light.position - vertex);

	/*	*/
	float dist = length(diffVertex);

	/*	*/
	float attenuation = 1.0 / (light.constant_attenuation + light.linear_attenuation * dist +
							   light.qudratic_attenuation * (dist * dist));

	float contribution = max(dot(normal, normalize(diffVertex)), 0.0);

	/*	*/
	vec4 pointLightColors = (attenuation * light.color * contribution * light.range * light.intensity);

	return vec4(pointLightColors.rgb, 1);
}

// Shadow.

float ShadowCalculation(const in sampler2DShadow ShadowTexture0, const in vec4 fragPosLightSpace, const in vec3 normal,
						const in vec3 lightDirection, const in float bias) {

	/*	perform perspective divide	*/
	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;
	/*	transform from NDC to Screen Space [0,1] range	*/
	projCoords = projCoords * 0.5 + 0.5;

	/*	*/
	const float ShadowBias = clamp(0.05 * (1.0 - dot(normalize(normal), normalize(lightDirection).xyz)), 0, bias);
	projCoords.z *= (1 - ShadowBias);

	/*	1 => shadow, 0 => lit.	*/
	const float shadow = textureProj(ShadowTexture0, projCoords).r;

	// if (projCoords.z > 1.0) {
	// 	shadow = 0.0;
	// }
	// if (fragPosLightSpace.w > 1) {
	// 	shadow = 0;
	// }

	return (1.0 - shadow);
}

// float ShadowCalculationPCF(const in vec4 fragPosLightSpace) {

// 	// perform perspective divide
// 	vec4 projCoords = fragPosLightSpace.xyzw / fragPosLightSpace.w;

// 	if (projCoords.w > 1.0) {
// 		return 0;
// 	}

// 	// transform to [0,1] range
// 	projCoords = projCoords * 0.5 + 0.5;

// 	const float bias =
// 		clamp(0.05 * (1.0 - dot(normalize(normal), normalize(ubo.directional.direction).xyz)), 0, ubo.bias);
// 	projCoords.z *= (1 - bias);

// 	float shadowFactor = 0;
// 	const ivec2 gMapSize = textureSize(ShadowTexture, 0);

// 	const float radius = 1.5;
// 	const float xOffset = 1.0 / gMapSize.x * radius;
// 	const float yOffset = 1.0 / gMapSize.y * radius;

// 	const float nrSamples = float(PCF_SAMPLES) * float(PCF_SAMPLES);

// 	/*	*/
// 	[[unroll]] for (int y = -PCF_SAMPLES / 2; y <= PCF_SAMPLES / 2; y++) {
// 		[[unroll]] for (int x = -PCF_SAMPLES / 2; x <= PCF_SAMPLES / 2; x++) {

// 			const vec2 Offsets = vec2(x * xOffset, y * yOffset);

// 			const vec3 UVC = vec3(projCoords.xy + Offsets, projCoords.z + EPSILON );
// 			shadowFactor += texture(ShadowTexture, UVC);
// 		}
// 	}

// 	return (1.0 - (shadowFactor / nrSamples));
// }

#endif
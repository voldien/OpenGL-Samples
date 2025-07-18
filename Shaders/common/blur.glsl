#ifndef _BLUR_COMMON_H_
#define _BLUR_COMMON_H_ 1

vec4 blurHorizontal(sampler2D ColorTexture, const in vec2 TexCoord, const in float[] kernels, const in float radius, const in int half_samples, const in int MAX_SAMPLES) {

	const vec2 resolution = textureSize(ColorTexture, 0);

	const vec2 texelSize = 1.0 / resolution;
	//const vec2 TexCoord = vec2(gl_GlobalInvocationID.xy) * texelSize;

	/*	Middle Sample.	*/
	vec4 color1 = texture(ColorTexture, TexCoord).rgba * vec4(settings.kernel[half_samples].xxx, 1.0);

	for(uint x = 1; x < half_samples; x++) {

		const vec2 uvP = TexCoord + vec2(0, texelSize.y * x) * radius;
		const vec2 uvN = TexCoord + vec2(0, -texelSize.y * x) * radius;

		const uint guassPIndex = clamp(half_samples + x, 0, MAX_SAMPLES - 1);
		const uint guassNIndex = clamp(half_samples - x, 0, MAX_SAMPLES - 1);

		const float guasP = kernels[guassPIndex];
		const float guasN = kernels[guassNIndex];

		color1 += texture(ColorTexture, uvP).rgba * vec4(guasP.xxx, 1.0);
		color1 += texture(ColorTexture, uvN).rgba * vec4(guasN.xxx, 1.0);
	}

	return color1;
}

vec4 blurVertical(const in float radius, const in int half_samples) {

	const vec2 resolution = textureSize(ColorTexture, 0);

	const vec2 texelSize = 1.0 / resolution;
	const vec2 TexCoord = vec2(gl_GlobalInvocationID.xy) * texelSize;

	/*	Middle Sample.	*/
	vec4 color1 = texture(ColorTexture, TexCoord).rgba * vec4(settings.kernel[half_samples].xxx, 1.0);

	for(uint x = 1; x < half_samples; x++) {
		const vec2 uvP = TexCoord + vec2(texelSize.x * x, 0) * radius;
		const vec2 uvN = TexCoord + vec2(-texelSize.x * x, 0) * radius;

		const uint guassPIndex = clamp(half_samples + x, 0, MAX_SAMPLES - 1);
		const uint guassNIndex = clamp(half_samples - x, 0, MAX_SAMPLES - 1);

		const float guasP = settings.kernel[guassPIndex];
		const float guasN = settings.kernel[guassNIndex];

		color1 += texture(ColorTexture, uvP).rgba * vec4(guasP.xxx, 1.0);
		color1 += texture(ColorTexture, uvN).rgba * vec4(guasN.xxx, 1.0);
	}

	return color1;
}

#endif

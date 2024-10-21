
struct FogSettings {
	/*	*/
	vec4 fogColor;
	/*	*/
	float CameraNear;
	float CameraFar;
	float fogStart;
	float fogEnd;
	/*	*/
	float fogDensity;
	uint fogType;
	float fogItensity;
	float fogHeight;
};

float getExpToLinear(const in float start, const in float end, const in float expValue) {
	return ((2.0f * start) / (end + start - expValue * (end - start)));
}

float getFogFactor(const in FogSettings fog_settings) {

	const float near = fog_settings.CameraNear;
	const float far = fog_settings.CameraFar;

	const float endFog = fog_settings.fogEnd / (far - near);
	const float startFog = fog_settings.fogStart / (far - near);
	const float densityFog = fog_settings.fogDensity;

	const float z = 0; // getExpToLinear(near, far, gl_FragCoord.z);

	switch (fog_settings.fogType) {
	case 1:
		return 1.0 - clamp((endFog - z) / (endFog - startFog), 0.0, 1.0);
	case 2:
		return 1.0 - clamp(exp(-(densityFog * z)), 0.0, 1.0);
	case 3:
		return 1.0 - clamp(exp(-(densityFog * z * densityFog * z)), 0.0, 1.0);
	// case 4:
	//	float dist = z * (far - near);
	//	float b = 0.5;
	//	float c = 0.9;
	//	return clamp(c * exp(-getCameraPosition().y * b) * (1.0 - exp(-dist * getCameraDirection().y * b)) /
	//					 getCameraDirection().y,
	//				 0.0, 1.0);
	default:
		return 0.0;
	}
}

vec4 blendFog(const in vec4 color, const in FogSettings fogSettings) {
	const float fog_factor = getFogFactor(fogSettings);

	return mix(color, fogSettings.fogColor, fog_factor);
}
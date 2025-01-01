#ifndef FOG_FRAG_
#define FOG_FRAG_ 1
#include "common.glsl"

float getFogFactor(const in FogSettings fog_settings, const float depth) {

	const float near = fog_settings.CameraNear;
	const float far = fog_settings.CameraFar;

	const float endFog = fog_settings.fogEnd / (far - near);
	const float startFog = fog_settings.fogStart / (far - near);
	const float densityFog = fog_settings.fogDensity;

	const float z = getExpToLinear(near, far, depth);

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

float getFogFactor(const in FogSettings fog_settings) { return getFogFactor(fog_settings, gl_FragCoord.z); }

vec4 blendFog(const in vec4 color, const in FogSettings fogSettings) {
	const float fog_factor = getFogFactor(fogSettings);

	return mix(color, fogSettings.fogColor, fog_factor);
}

#endif
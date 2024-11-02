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

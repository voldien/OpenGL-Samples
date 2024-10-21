float computeLightContributionFactor(const in vec3 direction, const in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}
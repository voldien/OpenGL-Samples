
struct DirectionalLight {
	vec4 direction;
	//float intensity;
	vec4 lightColor;
};

struct PointLight{
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

float computeLightContributionFactor(const in vec3 direction, const in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}
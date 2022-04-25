#version 450
#extension GL_ARB_separate_shader_objects : enable

float computeLightContributionFactor(in vec3 direction, in vec3 normalInput) {
	return max(0.0, dot(-normalInput, direction));
}

// TOOD exclude
void main() {}
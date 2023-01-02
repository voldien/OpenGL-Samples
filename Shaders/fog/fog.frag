#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 tangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;

	/*  Fog Settings.   */
	vec4 fogColor;
	float CameraNear;
	float CameraFar;
	float fogStart;
	float fogEnd;
	float fogDensity;

	uint fogType;
	float fogItensity;
}
ubo;

layout(binding = 1) uniform sampler2D DiffuseTexture;

/**
 *	Get exp function to linear.
 */
float getExpToLinear(const in float start, const in float end, const in float expValue) {
	return ((2.0f * start) / (end + start - expValue * (end - start)));
}

float getFogFactor() {

	const float near = ubo.CameraNear;
	const float far = ubo.CameraFar;

	const float endFog = ubo.fogEnd / (far - near);
	const float startFog = ubo.fogStart / (far - near);
	const float densityFog = ubo.fogDensity;

	const float z = getExpToLinear(near, far, gl_FragCoord.z);

	switch (ubo.fogType) {
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

void main() {
	float fogFactor = getFogFactor() * ubo.fogItensity;

	float contribution = max(0.0, dot(normalize(normal), normalize(ubo.direction.xyz)));

	fragColor = texture(DiffuseTexture, uv) * (ubo.ambientColor + ubo.lightColor * contribution);

	fragColor = mix(fragColor, ubo.fogColor, fogFactor).rgba;
}
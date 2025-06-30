#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

/*  */
layout(location = 0) out vec4 fragColor;
/*  */
layout(location = 0) in vec2 screenUV;

/*  */
layout(set = 0, binding = 0) uniform sampler2D ColorTexture;
layout(set = 0, binding = 1) uniform sampler2D DepthTexture;

#include "light.glsl"
#include "math.glsl"
#include "postprocessing_base.glsl"

layout(set = 0, binding = 1, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 viewRotation;
	Camera camera;
	FogSettings fogSettings;
}
ubo;

layout(push_constant) uniform Settings {
	layout(offset = 0) BaseSettings base;
	layout(offset = 4) float density; // Scattering density
	layout(offset = 8) float betaR;	  // Rayleigh scattering coefficient
	layout(offset = 12) float betaM;  // Mie scattering coefficient
	layout(offset = 16) float g;	  // Mie phase function parameter
	layout(offset = 32) DirectionalLight light;
}
settings;

vec3 calculate_atmospheric_scattering(const in vec3 vPos, const in vec3 cameraPos) {

	/*  */
	const vec3 viewPosDir = normalize(cameraPos - vPos);
	const vec3 L = settings.light.direction.xyz;

	/*  Rayleigh and Mie components */
	const vec3 rayleighComponent = rayleigh_phase(L, viewPosDir, settings.betaR);
	const float mieComponent = mie_phase(dot(L, viewPosDir), settings.g, settings.betaM);

	/*Total scattering contribution */
	return (rayleighComponent + mieComponent) * settings.density;
}

vec3 calcViewPosition(const in vec2 coords) {
	const float fragmentDepth = texture(DepthTexture, coords).r;

	return calcViewPosition(coords, constantCommon.constant.camera.inverseProj, fragmentDepth);
}

void main() {

	const vec3 viewPos = calcViewPosition(screenUV);
	const vec3 scattering = calculate_atmospheric_scattering(viewPos, constantCommon.constant.camera.position.xyz);

	fragColor.rgb = texture(ColorTexture, screenUV).rgb * scattering;
	fragColor.a = 1;
}

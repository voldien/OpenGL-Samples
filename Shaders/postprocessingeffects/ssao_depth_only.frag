#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision highp float;
precision mediump int;

layout(location = 1) out vec4 fragColor;
layout(location = 0) in vec2 screenUV;

layout(binding = 2) uniform sampler2D DepthTexture;
layout(binding = 4) uniform sampler2D NormalRandomize;

#include "postprocessing_base.glsl"

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	int samples;
	float radius;
	float intensity;
	float bias;
	vec4 kernel[128];
}
ubo;

vec3 calcViewPosition(const in vec2 coords) {
	const float fragmentDepth = texture(DepthTexture, coords).r;

	return calcViewPosition(coords, constantCommon.constant.camera.inverseProj, fragmentDepth);
}

void main() {
	const vec3 viewPos = calcViewPosition(screenUV);

	/*	*/
	const vec2 screenSize = textureSize(DepthTexture, 0).xy;
	const vec2 noiseScale = screenSize / textureSize(NormalRandomize, 0).xy;

	/*	*/
	vec3 viewNormal = cross(dFdy(viewPos.xyz), dFdx(viewPos.xyz));
	viewNormal = normalize(viewNormal * -1.0);

	// we calculate a random offset using the noise texture sample.
	// This will be applied as rotation to all samples for our current fragments.
	const vec3 randomVec = texture(NormalRandomize, screenUV * noiseScale).xyz;

	// here we apply the Gramm-Schmidt process to calculate the TBN matrix
	// with a random offset applied.
	const vec3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
	const vec3 bitangent = cross(viewNormal, tangent);
	const mat3 TBN = mat3(tangent, bitangent, viewNormal);

	float occlusion_factor = 0.0;

	const float kernelRadius = ubo.radius;
	const int samples = clamp(ubo.samples, 1, 128);

	for (uint i = 0; i < samples; i++) {

		const vec3 sampleWorldDir = TBN * ubo.kernel[i].xyz;

		/*	here we calculate the sampling point position in view space.	*/
		const vec3 samplePos = viewPos + sampleWorldDir * ubo.radius;

		/*	From view to clip-space.	*/
		vec4 offset = vec4(samplePos, 1.0);
		offset = constantCommon.constant.camera.proj * offset;
		offset.xyz /= offset.w;				 // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		// this is the geometry's depth i.e. the view_space_geometry_depth
		// this value is negative in my coordinate system
		const float geometryDepth = calcViewPosition(offset.xy).z;

		const float rangeCheck = smoothstep(0.0, 1.0, ubo.radius / abs(viewPos.z - geometryDepth));
		occlusion_factor += float(geometryDepth >= samplePos.z + ubo.bias) * rangeCheck;
	}

	// we will devide the accmulated occlusion by the number of samples to get the average occlusion value.
	const float average_occlusion_factor = occlusion_factor / samples;

	float visibility_factor = 1.0 - average_occlusion_factor * ubo.intensity;

	visibility_factor = pow(visibility_factor, 2.0);

	fragColor = vec4(visibility_factor.xxx, 1);
}

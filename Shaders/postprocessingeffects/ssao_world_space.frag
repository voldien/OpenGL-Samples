#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

precision highp float;
precision mediump int;

layout(location = 1) out float fragColor;
layout(location = 0) in vec2 screenUV;

layout(binding = 1) uniform sampler2D WorldTexture;	   /*	*/
layout(binding = 2) uniform sampler2D DepthTexture;	   /*	*/
layout(binding = 3) uniform sampler2D NormalTexture;   /*	*/
layout(binding = 4) uniform sampler2D NormalRandomize; /*	*/

#include "postprocessing_base.glsl"

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	int samples;
	float radius;
	float intensity;
	float bias;
	vec4 kernel[128];
}
ubo;

void main() {

	/*	*/
	const vec2 screenSize = textureSize(WorldTexture, 0).xy;
	const vec2 noiseScale = screenSize / vec2(textureSize(NormalRandomize, 0).xy);

	/*	*/
	const vec3 srcPosition = texture(WorldTexture, screenUV).xyz;
	const vec3 srcNormal = normalize(texture(NormalTexture, screenUV).xyz);
	const vec3 randVec = texture(NormalRandomize, screenUV * noiseScale).xyz;

	/*	Normal.	*/
	const vec3 tangent = normalize(randVec - srcNormal * dot(randVec, srcNormal));
	const vec3 bitangent = cross(srcNormal, tangent);
	const mat3 TBN = mat3(tangent, bitangent, srcNormal);

	/*	*/
	const float kernelRadius = ubo.radius;
	const int samples = clamp(ubo.samples, 1, 64);

	/*	*/
	float occlusion_factor = 0.0;

	for (uint i = 0; i < samples; i++) {

		/*	from tangent to view-space */
		const vec3 sampleWorldDir = TBN * ubo.kernel[i].xyz;

		/*	*/
		const vec3 samplePos = srcPosition + (sampleWorldDir * kernelRadius);

		/*	From view to clip-space.	*/
		vec4 offset = vec4(samplePos, 1.0);
		offset = constantCommon.constant.camera.proj * offset;
		offset.xyz /= offset.w;				 // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		const float sampleDepth = texture(WorldTexture, offset.xy).z;

		/*	*/
		const float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(srcPosition.z - sampleDepth));

		occlusion_factor += ((sampleDepth >= srcPosition.z + ubo.bias ? 1.0 : 0.0) * rangeCheck);
	}

	/* Average and clamp ambient occlusion_factor	*/
	occlusion_factor = (occlusion_factor / float(samples));

	occlusion_factor = 1.0 - occlusion_factor * ubo.intensity;

	occlusion_factor = pow(occlusion_factor, 2.0);

	fragColor = occlusion_factor;
}

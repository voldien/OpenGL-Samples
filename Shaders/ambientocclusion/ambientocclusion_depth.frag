#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out float fragColor;

layout(location = 0) in vec2 uv;

layout(binding = 1) uniform sampler2D WorldTexture;
layout(binding = 2) uniform sampler2D DepthTexture;
layout(binding = 3) uniform sampler2D NormalTexture;
layout(binding = 4) uniform sampler2D NormalRandomize;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 proj;
	int samples;
	float radius;
	float intensity;
	float bias;
	float cameraNear;
	float cameraFar;
	vec2 screen;

	vec3 kernel[64];

	vec4 color;
}
ubo;

float getExpToLinear(const in float start, const in float end, const in float expValue) {
	return ((2.0 * start) / (end + start - expValue * (end - start)));
}

void main() {

	const vec2 noiseScale = ubo.screen / vec2(textureSize(NormalRandomize, 0).xy);

	/*	*/
	const vec3 srcPosition = texture(WorldTexture, uv).xyz;
	const vec3 srcNormal = texture(NormalTexture, uv).rgb;
	const vec3 randVec = texture(NormalRandomize, uv * (ubo.screen / 3.0)).xyz;

	/*	*/
	const vec3 tangent = normalize(randVec - srcNormal * dot(randVec, srcNormal));
	const vec3 bitangent = cross(srcNormal, tangent);
	const mat3 TBN = mat3(tangent, bitangent, srcNormal);

	/*	*/
	// float srcDepth = texture(DepthTexture, uv).r;

	/*	*/
	const float kernelRadius = ubo.radius;
	const int samples = clamp(ubo.samples, 1, 64);

	float occlusion = 0.0;

	for (int i = 0; i < ubo.samples; i++) {
		vec3 samplePos = TBN * (ubo.kernel[i]); // from tangent to view-space
		samplePos = srcPosition + samplePos * kernelRadius;

		vec4 offset = vec4(samplePos, 1.0);
		offset = ubo.proj * offset;			 // from view to clip-space
		offset.xyz /= offset.w;				 // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		float bias = 0.045f;
		float sampleDepth = texture(DepthTexture, offset.xy).z;

		const float srcDepth = getExpToLinear(ubo.cameraNear, ubo.cameraFar, sampleDepth);

		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(srcPosition.z - sampleDepth));
		occlusion += (sampleDepth >= srcPosition.z + bias ? 1.0 : 0.0) * rangeCheck * ubo.intensity;
	}

	/* Average and clamp ambient occlusion	*/
	occlusion = 1.0 - (occlusion / float(samples)) * ubo.intensity;
	fragColor = occlusion;
}

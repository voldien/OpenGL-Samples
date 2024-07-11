#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out float fragColor;

layout(location = 0) in vec2 uv;

layout(binding = 1) uniform sampler2D WorldTexture;	   /*	*/
layout(binding = 2) uniform sampler2D DepthTexture;	   /*	*/
layout(binding = 3) uniform sampler2D NormalTexture;   /*	*/
layout(binding = 4) uniform sampler2D NormalRandomize; /*	*/

layout(set = 0, binding = 0, std140) uniform UniformBufferBlock {
	mat4 proj;
	int samples;
	float radius;
	float intensity;
	float bias;

	vec4 kernel[64];
	vec4 color;
	vec2 screen;
}
ubo;

void main() {

	/*	*/
	const vec2 noiseScale = ubo.screen / vec2(textureSize(NormalRandomize, 0).xy);

	/*	*/
	const vec3 srcPosition = texture(WorldTexture, uv).xyz;
	const vec3 srcNormal = normalize(texture(NormalTexture, uv).rgb);
	const vec3 randVec = texture(NormalRandomize, uv * noiseScale).xyz;

	/*	*/
	const vec3 tangent = normalize(randVec - srcNormal * dot(randVec, srcNormal));
	const vec3 bitangent = cross(srcNormal, tangent);
	const mat3 TBN = mat3(tangent, bitangent, srcNormal);

	/*	*/
	const float kernelRadius = ubo.radius;
	const int samples = clamp(ubo.samples, 1, 64);

	/*	*/
	float occlusion = 0.0;

	for (int i = 0; i < samples; i++) {

		/*	from tangent to view-space */
		const vec3 sampleWorldDir = TBN * ubo.kernel[i].xyz;
		/*	*/
		const vec3 samplePos = srcPosition + (sampleWorldDir * kernelRadius);

		/*	From view to clip-space.	*/
		vec4 offset = vec4(samplePos, 1.0);
		offset = ubo.proj * offset;
		offset.xyz /= offset.w;				 // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		const float sampleDepth = texture(WorldTexture, offset.xy).z;

		/*	*/
		const float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(srcPosition.z - sampleDepth));

		occlusion += ((sampleDepth >= srcPosition.z + ubo.bias ? 1.0 : 0.0) * rangeCheck);
	}

	/* Average and clamp ambient occlusion	*/
	occlusion = 1.0 - (occlusion / float(samples)) * ubo.intensity;
	fragColor = occlusion;
}

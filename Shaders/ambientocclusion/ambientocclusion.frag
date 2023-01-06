#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) out vec4 fragColor;

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
//	float OccluderBias = 0.045;

float SamplePixels(in vec3 srcPosition, in vec3 srcNormal, in vec2 uv) {

	float OccluderBias = 0.045;
	vec2 Attenuation = vec2(0.82999289, 0.0020000297);
	// Get the 3D position of the destination pixel
	// vec3 dstPosition = texture2D(WorldTexture, uv).xyz;

	// Calculate ambient occlusion amount between these two points
	// It is simular to diffuse lighting. Objects directly above the fragment cast
	// the hardest shadow and objects closer to the horizon have minimal effect.
	vec3 positionVec = texture(WorldTexture, uv).xyz - srcPosition;
	float intensity = max(dot(normalize(positionVec), srcNormal) - OccluderBias, 0.0);

	// Attenuate the occlusion, similar to how you attenuate a light source.
	// The further the distance between points, the less effect AO has on the fragment.
	float dist = length(positionVec);
	float attenuation = 1.0 / (Attenuation.x + (Attenuation.y * dist));

	return intensity * attenuation * ubo.intensity;
}

float getExpToLinear(const in float start, const in float end, const in float expValue) {
	return ((2.0f * start) / (end + start - expValue * (end - start)));
}

void main() {

	/*	*/
	vec3 srcPosition = texture(WorldTexture, uv).xyz;
	vec3 srcNormal = texture(NormalTexture, uv).rgb;
	vec3 randVec = texture(NormalRandomize, uv * (ubo.screen / 3.0)).xyz;

	/*	*/
	vec3 tangent = normalize(randVec - srcNormal * dot(randVec, srcNormal));
	vec3 bitangent = cross(srcNormal, tangent);
	mat3 TBN = mat3(tangent, bitangent, srcNormal);

	/*	*/
	// float srcDepth = texture(DepthTexture, uv).r;
	// srcDepth = getExpToLinear(ubo.cameraNear, ubo.cameraFar, srcDepth);

	float kernelRadius = ubo.radius; // * (1.0 - srcDepth);

	float occlusion = 0.0;

	for (int i = 0; i < ubo.samples; i++) {
		vec3 samplePos = TBN * (ubo.kernel[i]); // from tangent to view-space
		samplePos = srcPosition + samplePos * kernelRadius;

		vec4 offset = vec4(samplePos, 1.0);
		offset = ubo.proj * offset;			 // from view to clip-space
		offset.xyz /= offset.w;				 // perspective divide
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		float bias = 0.045f;
		float sampleDepth = texture(WorldTexture, offset.xy).z;

		float rangeCheck = smoothstep(0.0, 1.0, kernelRadius / abs(srcPosition.z - sampleDepth));
		occlusion += (sampleDepth >= srcPosition.z + bias ? 1.0 : 0.0) * rangeCheck * ubo.intensity;
	}

	/* Average and clamp ambient occlusion	*/
	// occlusion /= (ubo.samples);
	occlusion = (1.0 - (occlusion / float(ubo.samples)));

	fragColor = vec4(occlusion, occlusion, occlusion, 1.0);
}

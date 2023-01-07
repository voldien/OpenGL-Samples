#version 460
#extension GL_ARB_separate_shader_objects : enable

/*  */
layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 UV;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec4 lightSpace;

layout(binding = 1) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2DShadow ShadowTexture;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	mat4 lightSpaceMatrix;
	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;
	vec4 subsurfaceColor;

	float bias;
	float shadowStrength;
	float sigma;
}
ubo;

float getExpToLinear(const in float nearZ, const in float farZ, const in float ndc_depth) {
	return ndc_depth; // (ndc_depth * (farZ - nearZ) + nearZ) / ( farZ - nearZ);
}

float trace() {
	vec4 projCoords = lightSpace / lightSpace.w;

	// transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	float bias = max(0.05 * (1.0 - dot(normalize(normal), -normalize(ubo.direction).xyz)), ubo.bias);
	projCoords.z *= (1 - bias);

	float d_i = textureProj(ShadowTexture, projCoords, 0).r;
	vec4 Plight = lightSpace / lightSpace.w;

	const float d_o = length(Plight.xyz);
	float s = (getExpToLinear(-30, 30, d_i) - getExpToLinear(-30, 30, d_o));
	// return tex1D(scatterTex, si);

	// if (projCoords.z > 1.0) {
	//	s = 0.0;
	//}
	// if (projCoords.w > 1) {
	//	s = 0;
	//}

	return exp(-s * ubo.sigma);
}

void main() {

	float contribution = max(dot(normalize(normal), -normalize(ubo.direction).xyz), 0.0);

	vec4 sub = (trace() * ubo.subsurfaceColor + contribution * ubo.lightColor + ubo.ambientColor);

	fragColor = texture(DiffuseTexture, UV) * sub;
}

// float si = trace(IN.objCoord, lightTexMatrix, lightMatrix,                  lightDepthTex); return tex1D(scatterTex,
// si);

// Given a point in object space, lookup into depth textures
// returns depth
//   float trace(float3 P,             uniform float4x4  lightTexMatrix, // to light texture space     uniform float4x4
//   lightMatrix,    // to light space     uniform sampler2D lightDepthTex,             ) {   // transform point into
//   light texture space     float4 texCoord = mul(lightTexMatrix, float4(P, 1.0));
// get distance from light at entry point     float d_i = tex2Dproj(lightDepthTex, texCoord.xyw);
// transform position to light space     float4 Plight = mul(lightMatrix, float4(P, 1.0));
// distance of this pixel from light (exit)     float d_o = length(Plight);
// calculate depth     float s = d_o - d_i;   return s; }
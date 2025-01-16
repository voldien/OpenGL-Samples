#include "common.glsl"
#include "light.glsl"
#include "material.glsl"

struct common_data {
	Camera camera;
	Frustum frustum;
};

struct Node {
	mat4 model;
};

layout(set = 1, binding = 0, std140) uniform UniformCommonBufferBlock {
	common_data constant;

	mat4 view;
	mat4 proj;
}
constantCommon;

layout(set = 1, binding = 2, std140) uniform UniformSkeletonBufferBlock { mat4 gBones[512]; }
skeletonUBO2;

layout(binding = 0) uniform sampler2D DiffuseTexture;
layout(binding = 1) uniform sampler2D NormalTexture;
layout(binding = 2) uniform sampler2D AlphaMaskedTexture;

layout(binding = 5) uniform sampler2D RoughnessTexture;
layout(binding = 6) uniform sampler2D MetalicTexture;
layout(binding = 4) uniform sampler2D EmissionTexture;
layout(binding = 7) uniform sampler2D DisplacementTexture;

layout(binding = 10) uniform sampler2D IrradianceTexture;
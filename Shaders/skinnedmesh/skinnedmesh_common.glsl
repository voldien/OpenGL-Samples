#include "common.glsl"
#include "light.glsl"

/*	*/
layout(constant_id = 16) const int MAX_BONES = 512;
layout(constant_id = 17) const int MAX_BONE_INFLUENCE = 4;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Light source.	*/
	DirectionalLight directional;

	vec4 ambientColor;

	/*	Material color.	*/
	vec4 tintColor;
}
ubo;

layout(binding = 1, std140) uniform UniformSkeletonBufferBlock { mat4 gBones[MAX_BONES]; }
skeletonUBO;
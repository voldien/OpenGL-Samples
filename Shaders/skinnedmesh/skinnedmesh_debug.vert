#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in uvec4 BoneIDs; /*	*/
layout(location = 5) in vec4 Weights;  /*	*/

layout(location = 0) out vec4 FragIN_weight;

#include "skinnedmesh_common.glsl"

float rand(const in vec2 co) { return abs(fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453)); }

void main() {

	mat4 BoneTransform = skeletonUBO.gBones[BoneIDs[0]] * Weights[0];
	BoneTransform += skeletonUBO.gBones[BoneIDs[1]] * Weights[1];
	BoneTransform += skeletonUBO.gBones[BoneIDs[2]] * Weights[2];
	BoneTransform += skeletonUBO.gBones[BoneIDs[3]] * Weights[3];

	const vec4 boneVertex = BoneTransform * vec4(Vertex, 1.0);

	gl_Position = ubo.modelViewProjection * boneVertex;

	FragIN_weight = vec4(rand(vec2(BoneIDs[0], 1)) * Weights[0], rand(vec2(BoneIDs[1], 1)) * Weights[1],
						 rand(vec2(BoneIDs[2], 1)) * Weights[2], 1);
}
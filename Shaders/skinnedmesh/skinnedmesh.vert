
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_control_flow_attributes : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in uvec4 BoneIDs; /*	*/
layout(location = 5) in vec4 Weights;  /*	*/

layout(location = 0) out vec3 FragIN_position;
layout(location = 1) out vec2 FragIN_uv;
layout(location = 2) out vec3 FragIN_normal;
layout(location = 3) out vec3 FragIN_tangent;

#include "skinnedmesh_common.glsl"


void main() {

	/*	*/
	vec4 deformedVertex = vec4(0.0f);
	vec3 deformedNormal = vec3(0.0);
	vec3 deformedTangent = vec3(0.0);
	[[unroll]] for (uint i = 0; i < MAX_BONE_INFLUENCE; i++) {

		if (BoneIDs[i] >= MAX_BONES) {
			deformedVertex = vec4(Vertex, 1.0f);
			break;
		}

		const vec4 localPosition = skeletonUBO.gBones[BoneIDs[i]] * vec4(Vertex, 1.0f);
		deformedVertex += localPosition * Weights[i];

		const vec3 localNormal = mat3(skeletonUBO.gBones[BoneIDs[i]]) * Normal;
		deformedNormal += localNormal * Weights[i];

		const vec3 localTangent = mat3(skeletonUBO.gBones[BoneIDs[i]]) * Tangent;
		deformedTangent += localTangent * Weights[i];
	}

	gl_Position = ubo.modelViewProjection * deformedVertex;
	FragIN_normal = normalize((ubo.model * vec4(deformedNormal, 0.0)).xyz);
	FragIN_tangent = normalize((ubo.model * vec4(deformedTangent, 0.0)).xyz);
	FragIN_uv = TextureCoord;
}

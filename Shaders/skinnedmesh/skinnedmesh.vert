
#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in ivec4 BoneIDs;
layout(location = 5) in vec4 Weights;

layout(location = 0) out vec3 FragIN_position;
layout(location = 1) out vec2 FragIN_uv;
layout(location = 2) out vec3 FragIN_normal;
layout(location = 3) out vec3 FragIN_tangent;

const int MAX_BONES = 200;
layout(binding = 0, std140) uniform UniformBufferBlock {

	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	/*	Tint color.	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

layout(binding = 0, std140) uniform UniformSkeletonBufferBlock { mat4 gBones[MAX_BONES]; }
skeletonUBO;

void main() {

	mat4 BoneTransform = skeletonUBO.gBones[BoneIDs[0]] * Weights[0];
	BoneTransform += skeletonUBO.gBones[BoneIDs[1]] * Weights[1];
	BoneTransform += skeletonUBO.gBones[BoneIDs[2]] * Weights[2];
	BoneTransform += skeletonUBO.gBones[BoneIDs[3]] * Weights[3];

	const vec4 boneVertex = BoneTransform * vec4(Vertex, 1.0);

	gl_Position = ubo.modelViewProjection * boneVertex;
	FragIN_normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	FragIN_uv = TextureCoord;
}
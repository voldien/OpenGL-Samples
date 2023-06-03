
#version 460
#extension GL_ARB_separate_shader_objects : enable
layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in ivec4 BoneIDs;
layout(location = 5) in vec4 Weights;

layout(location = 0) out vec2 FragIN_uv;
layout(location = 1) out vec3 FragIN_normal;

const int MAX_BONES = 200;
layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	mat4 gBones[MAX_BONES];

	/*	Tint color.	*/
	vec4 tintColor;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
}
ubo;

void main() {
	mat4 BoneTransform = ubo.gBones[BoneIDs[0]] * Weights[0];
	BoneTransform += ubo.gBones[BoneIDs[1]] * Weights[1];
	BoneTransform += ubo.gBones[BoneIDs[2]] * Weights[2];
	BoneTransform += ubo.gBones[BoneIDs[3]] * Weights[3];

	vec4 boneVertex = BoneTransform * vec4(Vertex, 1.0);

	gl_Position = ubo.modelViewProjection * boneVertex;
	FragIN_normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	FragIN_uv = TextureCoord;
}
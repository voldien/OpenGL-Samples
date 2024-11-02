#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) out flat int InstanceID;

#include "deferred_base.glsl"
#include "light.glsl"

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;

	Camera camera;
}
ubo;

layout(set = 0, binding = 1, std140) uniform UniformBufferLight { PointLight point_light[64]; }
pointlightUBO;

void main() {

	InstanceID = int(gl_InstanceID);

	const float range = pointlightUBO.point_light[InstanceID].range;
	const vec3 position = pointlightUBO.point_light[InstanceID].position;
	
	mat4 transform = mat4(1);
	transform[3][0] = position.x;
	transform[3][1] = position.y;
	transform[3][2] = position.z;

	transform[0][0] = range;
	transform[1][1] = range;
	transform[2][2] = range;

	gl_Position = (ubo.proj * ubo.view * transform) * vec4(Vertex, 1);
}

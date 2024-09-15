#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;

layout(location = 1) out flat int InstanceID;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
}
ubo;

struct point_light {
	vec3 position;
	float range;
	vec4 color;

	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

layout(set = 0, binding = 1, std140) uniform UniformBufferLight { point_light point_light[64]; }
pointlightUBO;

void main() {

	InstanceID = int(gl_InstanceID);

	const float range = pointlightUBO.point_light[InstanceID].range;
	const vec3 position = pointlightUBO.point_light[InstanceID].position;

	mat4 m =
		mat4(range, 0.0, 0.0, position.x, 0.0, range, 0.0, position.y, 0.0, 0.0, range, position.z, 0.0, 0.0, 0.0, 1.0);
	// m[0] = vec4(range, 0, 0, position.x);
	// m[1] = vec4(0, range, 0, position.y);
	// m[2] = vec4(0, 0, range, position.z);
	// m[3] = vec4(0, 0, 0, 1.0);

	mat4 transform = mat4(1);
	transform[3][0] = position.x;
	transform[3][1] = position.y;
	transform[3][2] = position.z;

	transform[0][0] = range;
	transform[1][1] = range;
	transform[2][2] = range;

	gl_Position = (ubo.proj * ubo.view * m) * vec4(Vertex, 1);
}

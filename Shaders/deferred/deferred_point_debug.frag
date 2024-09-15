#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fragColor;

layout(location = 1) in flat int InstanceID;

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

	fragColor = pointlightUBO.point_light[InstanceID].color;
	fragColor.a = 1;
}
#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) out vec4 fragColor;

layout(location = 1) in flat int InstanceID;

#include"deferred_base.glsl"

layout(set = 0, binding = 1, std140) uniform UniformBufferLight { PointLight point_light[64]; }
pointlightUBO;

void main() {

	fragColor = pointlightUBO.point_light[InstanceID].color;
	fragColor.a = 1;
}

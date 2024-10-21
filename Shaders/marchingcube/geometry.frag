#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_EXT_control_flow_attributes : enable

layout(location = 0) in vec2 coord;
layout(location = 1) in vec3 color;

layout(location = 0) out vec4 fragColor;

#include "marchinbcube_common.glsl"

void main() {
	fragColor = vec4(color, 1);
	fragColor = blendFog(fragColor, ubo.fogSettings);
}

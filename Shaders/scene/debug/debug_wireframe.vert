#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
/*	*/
layout(location = 8) in ivec2 vAssigns;

layout(location = 0) out vec3 color;

#include "scene.glsl"

void main() {

	const mat4 model = getModel(vAssigns.y);
	const mat4 viewProj = getCamera().viewProj;

	gl_Position = (viewProj * model) * vec4(Vertex, 1.0);

    color = vec3(1,1,1);
}
#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;

layout(location = 3) out vec4 instanceColor;

#include "debug_drawer.glsl"

void main() {
	gl_Position = ubo.proj * ubo.view * instance_ubo.ins[gl_InstanceID].model * vec4(Vertex, 1.0);
	instanceColor = instance_ubo.ins[gl_InstanceID].color;
}
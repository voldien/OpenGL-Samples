#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(points) in;
layout(line_strip) out;
layout(max_vertices = 6) out;

layout(location = 1) smooth in vec3 normal[];
layout(location = 2) smooth in vec3 tangent[];

layout(location = 0) smooth out vec4 normalColor;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProj;
	mat4 modelViewProjection;

	float normalLength;
}
ubo;

void main() {

	const vec3 axis[3] = {normal[0], tangent[0], cross(normalize(tangent[0]), normalize(normal[0]))};
	const vec4 axisColor[3] = {vec4(0, 0, 1, 0.8), vec4(1, 0, 0, 0.8), vec4(0, 1, 0, 0.8)};

	[[unroll]] for (uint i = 0; i < 3; i++) {
		gl_Position = ubo.ViewProj * vec4(gl_in[0].gl_Position.xyz, 1.0);
		normalColor = axisColor[i];
		EmitVertex();

		gl_Position = ubo.ViewProj * vec4(gl_in[0].gl_Position.xyz + normalize(axis[i]) * ubo.normalLength, 1.0);
		normalColor = axisColor[i];
		EmitVertex();

		EndPrimitive();
	}
}
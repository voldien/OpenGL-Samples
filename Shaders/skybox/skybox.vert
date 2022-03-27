#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertex;
layout(location = 0) out vec3 vVertex;

/**
 *
 */
layout(set = 0, binding = 0) uniform params {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ModelViewProjection;
	vec4 DiffuseColor;
} params_;


void main() {
	vec4 MVPPos = params_.ModelViewProjection * vec4(vertex, 1.0);
	gl_Position = MVPPos.xyww;
	vVertex = vertex;
}
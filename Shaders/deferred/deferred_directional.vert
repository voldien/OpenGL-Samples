#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_include : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 Vertex;
layout(location = 2) out vec2 uv;
layout(location = 1) out flat int InstanceID;

#include"deferred_base.glsl"

void main() {
	gl_Position = vec4(Vertex.x, Vertex.y, 0, 1.0);

	uv = (vec2(Vertex.x, Vertex.y) + vec2(1, 1)) / 2.0;
	InstanceID = int(gl_InstanceID);
}

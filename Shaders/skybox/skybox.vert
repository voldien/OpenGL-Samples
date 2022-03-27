#version 450

layout(location = 0) in vec3 vertex;
layout(location = 0) out vec3 vVertex;

layout(location = 0) uniform mat4 MVP;

void main() {
	vec4 MVPPos = MVP * vec4(vertex, 1.0);
	gl_Position = MVPPos.xyww;
	vVertex = vertex;
}
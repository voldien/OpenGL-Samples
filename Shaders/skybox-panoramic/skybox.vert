#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 vertex;
layout(location = 0) out vec3 vVertex;

layout(binding = 0) uniform UniformBufferBlock { mat4 MVP;  }
ubo;
void main() {
	vec4 MVPPos = ubo.MVP * vec4(vertex, 1.0);
	gl_Position = MVPPos.xyww;
	vVertex = normalize(vertex);
}
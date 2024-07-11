#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;

layout(location = 0) out vec3 vVertex;

layout(set = 0, binding = 1, std140) uniform UniformBufferBlock {
	mat4 proj;
	mat4 modelViewProjection;
	vec4 tintColor;
	float exposure;
	float gamma;
}
ubo;

void main() {
	vec4 MVPPos = ubo.modelViewProjection * vec4(Vertex, 1.0);
	gl_Position = MVPPos.xyww;
	vVertex = Vertex;
}
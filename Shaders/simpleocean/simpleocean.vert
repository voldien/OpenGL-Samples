#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 UV;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 tangent;

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	/*	Light source.	*/
	vec4 lookDirection;
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 position;

	float time;
	float freq;
	float amplitude;
}
ubo;

void main() {

	// TOOD recompute the normals and tangent
	vertex = (ubo.model * vec4(Vertex, 1.0)).xyz;
	normal = normalize((ubo.model * vec4(Normal, 0.0)).xyz);
	tangent = (ubo.model * vec4(Tangent, 0.0)).xyz;


	float height = cos(ubo.time + vertex.x * ubo.freq) + sin(ubo.time + vertex.z * ubo.freq);

	vertex = vertex + (normalize(normal) * height);
	gl_Position = ubo.modelViewProjection * vec4(vertex, 1.0);

	UV = TextureCoord;
}
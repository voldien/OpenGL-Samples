#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 Vertex;
layout(location = 1) in vec2 TextureCoord;
layout(location = 2) in vec3 Normal;

layout(location = 0) out vec3 vertex;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 normal;

struct spot_light {
	vec4 position;
	vec4 color;
	vec4 direction;
	float range;
	float angle;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 modelViewProjection;
	/*	Light source.	*/
	vec4 ambientColor;
	spot_light spot_light[4];
}
ubo;

void main() {
	gl_Position = ubo.modelViewProjection * vec4(Vertex, 1.0);
	vertex = (ubo.model * vec4(Vertex, 1.0)).xyz;
	normal = (ubo.model * vec4(Normal, 0.0)).xyz;
	uv = TextureCoord;
}
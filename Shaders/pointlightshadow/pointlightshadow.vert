#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(location = 0) in vec3 vertex;

struct point_light {
	vec3 position;
	float range;
	vec4 color;
	float intensity;
	float constant_attenuation;
	float linear_attenuation;
	float qudratic_attenuation;
	float bias;
	float shadowStrength;
};

layout(binding = 0, std140) uniform UniformBufferBlock {
	mat4 model;
	mat4 view;
	mat4 proj;
	mat4 modelView;
	mat4 ViewProjection[6];
	mat4 modelViewProjection;

	/*	Light source.	*/
	vec4 direction;
	vec4 lightColor;
	vec4 ambientColor;
	vec4 cameraPosition;

	point_light point_light[4];
}
ubo;

void main() { gl_Position = ubo.model * vec4(vertex, 1.0); }
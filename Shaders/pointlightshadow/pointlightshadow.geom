#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) out vec4 FragPos;

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

void main() {

	for (int face = 0; face < 6; ++face) {
		gl_Layer = face;			// built-in variable that specifies to which face we render.
		for (int i = 0; i < 3; ++i) // for each triangle vertex
		{
			FragPos = gl_in[i].gl_Position;
			gl_Position = (ubo.ViewProjection[gl_Layer]) * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}
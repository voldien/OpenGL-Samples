#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_explicit_attrib_location : enable
#extension GL_ARB_uniform_buffer_object : enable
#extension GL_EXT_control_flow_attributes : enable

layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 0) in flat int GIndex[];
layout(location = 1) in vec2 InTextureCoord[];

layout(location = 0) out vec4 FragVertex;
layout(location = 2) out vec2 FragTextureCoord;
layout(location = 1) out invariant flat int FIndex;

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
	float padding0;
	float padding1;
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
	vec4 PCFFilters[20];
	float diskRadius;
	int samples;
}
ubo;

void main() {

	[[unroll]] for (int face = 0; face < 6; ++face) {
		gl_Layer = face;						// built-in variable that specifies to which face we render.
		[[unroll]] for (uint i = 0; i < 3; ++i) // for each triangle vertex
		{
			FragVertex = gl_in[i].gl_Position;
			FIndex = GIndex[i];
			FragTextureCoord = InTextureCoord[i];
			gl_Position = (ubo.ViewProjection[gl_Layer]) * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}